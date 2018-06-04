//
// Created by jdecombe on 14/05/18.
//

#include <cmath>
#include <cstddef>
#include <string>
#include <iostream>
#include <future>
#include <utility>
#include "Gfx.hpp"
#include "Player.hpp"
#include "Bomb.hpp"
#include "CustomEventReceiver.hpp"


namespace object {
	/*
	 * Construction/Destructions
	 */

	static std::unordered_map<playerSprite_e, playerSpriteInfo_t> playersPossibilities = {
		{POMMY, {"./media/models/pommy/pommyV3.obj", {2.f, 2.f, 2.f}}},
		{UNKNOWN, {"", {0, 0, 0}}}};

	Player::Player(::map::Map *map, Type type, Gfx *gfx, playerSprite_e playerType, float x, float y,
		float z, bool useController, unsigned long playerNb
	) : AObject(type), _map(map), _gfx(gfx), _useController(useController), _playerNb(playerNb),
		_playerNode(nullptr)
	{

		auto modelInfo = playersPossibilities[playerType];
		this->_playerNode = this->_gfx->drawMesh(modelInfo.path, "",
			false, "Player" + this->_playerNb, {x, y, z});
		this->_playerNode->setScale(modelInfo.scale);
		this->_rotateStop = false;
	}

	Player::~Player()
	{
		if (_rotationThread.joinable())
			_rotationThread.join();
		this->_gfx->deleteElement("Player" + this->_playerNb);
	}

	void Player::update() noexcept
	{
		if (_gfx->isKeyDown(KEY_KEY_D))
			move(LEFT);
		else if (_gfx->isKeyDown(KEY_KEY_Q))
			move(RIGHT);
		else if (_gfx->isKeyDown(KEY_KEY_Z))
			move(FORWARD);
		else if (_gfx->isKeyDown(KEY_KEY_S))
			move(BACKWARD);

		if (_gfx->isKeyDown(KEY_SPACE))
			placeBomb();
	}

	void Player::updatePosition(size_t oldx, size_t oldy) noexcept
	{
		auto position = getPosition();

		if ((position.X != oldx || position.Y != oldy))
			_map->movePlayer(getType(), oldx, oldy, position.X, position.Y);
	}

	void Player::bombExploaded() noexcept
	{
		_placedBombs--;
		if (_placedBombs < 0)
			_placedBombs = 0;
	}

	void Player::placeBomb() noexcept
	{
		auto pos = getPosition();

		if (_placedBombs >= _maxBombs)
			return;

		if (_map->getTileAt(pos.X, pos.Y)->containsObject(BOMB))
			return;

		_placedBombs++;
		if (_placedBombs > _maxBombs)
			_placedBombs = _maxBombs;

		auto bomb = std::make_unique<object::Bomb>(pos.X, pos.Y, _blastRadius, _map, _gfx, this);
		_map->addObjectToTile(pos.X, pos.Y, std::move(bomb));
	}

	/*
	 * Rotations
	 */

	void Player::rotateThreadHandler(float maxAngle, float curAngle,
		std::string name
	)
	{
		auto step = (maxAngle - curAngle) / abs(maxAngle - curAngle);

		while ((step > 0 && curAngle < maxAngle) ||
			(step < 0 && curAngle > maxAngle)) {
			if (_rotateStop) {
				_rotateStop = false;
				return;
			}
			curAngle += step;
			_gfx->rotateElement(name, {0, curAngle, 0});
			std::this_thread::sleep_for(
				std::chrono::milliseconds(1));
		}
		_gfx->rotateElement(name, {0, maxAngle, 0});
	}

	static float getAngleFromDirection(enum rotationDirection_e rotation)
	{
		float angle = 0.f;

		switch (rotation) {
		case RIGHT:
			angle = 90;
			break;
		case LEFT:
			angle = -90;
			break;
		case BACKWARD:
			angle = 0;
			break;
		case FORWARD:
			angle = 180;
			break;
		}
		return angle;
	}

	void Player::setAbsoluteRotation(enum rotationDirection_e rotation)
	{
		auto angle = getAngleFromDirection(rotation);
		this->setAbsoluteRotation(angle);
	}

	void Player::setAbsoluteRotation(float maxAngle)
	{
		auto curAngle = this->_playerNode->getRotation().Y;
		curAngle = static_cast<float>(fmod(curAngle, 360));
		if (std::abs(maxAngle - curAngle) > 240) {
			if (curAngle < maxAngle)
				curAngle += 360;
			else
				maxAngle += 360;
		}
		auto name = std::string("Player") + std::to_string(_playerNb);
		if (_rotationThread.joinable()) {
			_rotateStop = true;
			_rotationThread.join();
		}
		_rotateStop = false;
		this->_rotationThread = std::thread(
			&Player::rotateThreadHandler, this, maxAngle, curAngle,
			"Player" + _playerNb);
	}

	/*
	 * Movements
	 */

	bool Player::hasCollided(vector2di tilePos) noexcept
	{
		auto position = getPosition();

		if (!(position.X == tilePos.X && position.Y == tilePos.Y)
			&& (_map->getTileAt(tilePos.X, tilePos.Y)->containsObject(WALL)
			|| _map->getTileAt(tilePos.X, tilePos.Y)->containsObject(BOX)
			|| _map->getTileAt(tilePos.X, tilePos.Y)->containsObject(BOMB)
			|| (_map->getTileAt(tilePos.X, tilePos.Y)->containsObject(PLAYER1) && getType() != PLAYER1)
			|| (_map->getTileAt(tilePos.X, tilePos.Y)->containsObject(PLAYER2) && getType() != PLAYER2)
			|| (_map->getTileAt(tilePos.X, tilePos.Y)->containsObject(PLAYER3) && getType() != PLAYER3)
			|| (_map->getTileAt(tilePos.X, tilePos.Y)->containsObject(PLAYER4) && getType() != PLAYER4)))
			return true;

		return false;
	}

	void Player::move(rotationDirection_e dir, float spd)
	{
		vector3df vec = {0, 0, 0};
		auto angle = getAngleFromDirection(dir);
		setAbsoluteRotation(angle);

		switch (dir) {
		case BACKWARD:
			if (_playerNode->getPosition().X >= 25)
				return;
			vec = {1., 0., 0.};
			break;
		case FORWARD:
			if (_playerNode->getPosition().X <= -25)
				return;
			vec = {-1., 0., 0.};
			break;
		case RIGHT:
			if (_playerNode->getPosition().Z <= -25)
				return;
			vec = {0., 0., -1.};
			break;
		case LEFT:
			if (_playerNode->getPosition().Z >= 25)
				return;
			vec = {0., 0., 1.};
			break;
		default:
			break;
		}

		vec *= spd;

		auto newPos = _playerNode->getPosition() + vec;
		newPos /= 4;
		newPos += 7.5f;
		vector2di tilePos = {static_cast<s32>(newPos.Z - 1),
			static_cast<s32>(newPos.X - 1)};

		if (hasCollided(tilePos))
			return;

		this->_gfx->moveElement("Player" + this->_playerNb, vec);
	}

	/*
	 * Getters
	 */

	vector2di Player::getPosition() noexcept
	{
		auto vectorFloat = this->_playerNode->getPosition() / 4;
		vectorFloat += 6.5f;
		vector2di value = {static_cast<s32>(vectorFloat.Z),
			static_cast<s32>(vectorFloat.X)};
		return value;
	}

	IAnimatedMeshSceneNode *Player::getNode() const
	{
		return _playerNode;
	}

	vector2df Player::getPositionFloat()
	{
		auto vectorFloat = this->_playerNode->getPosition() / 4;
		vectorFloat += 6.5f;
		vector2df value = {vectorFloat.Z, vectorFloat.X};
		return value;
	}
}
