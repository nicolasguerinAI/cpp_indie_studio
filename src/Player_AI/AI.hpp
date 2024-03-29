/*
** EPITECH PROJECT, 2021
** IndieStudio
** File description:
** Created by tclemenceau,
*/

#ifndef INDIESTUDIO_AI_HPP
#define INDIESTUDIO_AI_HPP

#include <random>
#include "Map.hpp"
#include "Player.hpp"

namespace object {

class AI : public AObject {
	public:
		AI(::map::Map *map, Type type, Gfx *gfx, playerSprite_e playerType, float x,
			float y, float z, bool useController,
			unsigned long playerNb);
		~AI();
		void update() noexcept;
		void updatePosition(size_t oldx, size_t oldy) noexcept;
		object::Player &getPlayer();
		irr::core::vector2di getPosition() noexcept { return _player.getPosition(); }
	private:
		void moveCase(rotationDirection_e dir, float spd = MOVEMENT_SPEED);
		void findClosestPlayer();
		void setWayToPlayer();
		bool checkMoveToTile(size_t x, size_t y, rotationDirection_e dir = NONE);
		bool moveToXPlayer(irr::core::vector2di tmp, bool invert);
		bool moveToYPlayer(irr::core::vector2di tmp, bool invert);
		void setSafeDestination(size_t x, size_t y);
		bool isSafe(size_t x, size_t y);
		bool checkIfSafeDestination(rotationDirection_e dir);
		bool checkBombDanger(size_t x, size_t y, rotationDirection_e dir = NONE);
		bool checkBombImpactAt(size_t x, size_t y, size_t xPlayer, size_t yPlayer) const;
		bool isBoxNearby(int x, int y, int bombSize);
		void move();
		irr::core::vector2di _closestPlayer;
		object::Player _player;
		::map::Map *_map;
		rotationDirection_e _destinationDirection;
		irr::core::vector2df _destinationCoordinates;
		std::random_device _random;
};

}

#endif //INDIESTUDIO_AI_HPP
