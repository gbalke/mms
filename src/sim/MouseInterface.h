#pragma once

#include "InterfaceType.h"
#include "MazeGraphic.h"
#include "Mouse.h"
#include "MouseInterfaceOptions.h"
#include "Param.h"

#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

#define ENSURE_DISCRETE_INTERFACE ensureDiscreteInterface(__func__);
#define ENSURE_CONTINUOUS_INTERFACE ensureContinuousInterface(__func__);
#define ENSURE_ALLOW_OMNISCIENCE ensureAllowOmniscience(__func__);

namespace manual {
    class Manual;
}

namespace sim {

class MouseInterface {

public:
    MouseInterface(
        const Maze* maze,
        Mouse* mouse,
        MazeGraphic* mazeGraphic,
        std::set<char> allowableTileTextCharacters,
        MouseInterfaceOptions options);

    // ----- Any interface methods ----- //

    // Logging functions
    void debug(const std::string& str);
    void info(const std::string& str);
    void warn(const std::string& str);
    void error(const std::string& str);

    // Misc functions
    double getRandom();
    int millis(); // # of milliseconds of sim time (adjusted based on sim speed) that have passed
    void delay(int milliseconds);
    void quit();

    // Tile color
    void setTileColor(int x, int y, char color);
    void clearTileColor(int x, int y);
    void clearAllTileColor();

    // Tile text
    void setTileText(int x, int y, const std::string& text);
    void clearTileText(int x, int y);
    void clearAllTileText();

    // Tile walls
    void declareWall(int x, int y, char direction, bool wallExists);
    void undeclareWall(int x, int y, char direction);

    // Tile fog
    void setTileFogginess(int x, int y, bool foggy);

    // Tile distance, where a negative distance corresponds to inf distance
    void declareTileDistance(int x, int y, int distance);
    void undeclareTileDistance(int x, int y);

    // Reset position of the mouse
    void resetPosition();

    // Input buttons
    bool inputButtonPressed(int inputButton);
    void acknowledgeInputButtonPressed(int inputButton);

    // ----- Continuous interface methods ----- //

    // Get the magnitude of the max speed of any one wheel in rpm
    double getWheelMaxSpeed(const std::string& name);

    // Set the speed of any one wheel
    void setWheelSpeed(const std::string& name, double rpm);

    // Get the number of encoder ticks per revolution for a wheel
    double getWheelEncoderTicksPerRevolution(const std::string& name);

    // Read the encoder for a particular wheel
    int readWheelEncoder(const std::string& name);

    // Reset the encoder for a particular wheel to zero, but only if the encoder type is relative
    void resetWheelEncoder(const std::string& name);

    // Returns a value in [0.0, 1.0]
    double readSensor(std::string name);

    // Returns deg/s of rotation
    double readGyro();

    // ----- Discrete interface methods ----- //

    bool wallFront();
    bool wallRight();
    bool wallLeft();

    void moveForward();
    void moveForward(int count);

    void turnLeft();
    void turnLeft(int count);

    void turnRight();
    void turnRight(int count);

    void turnAround();
    void turnAround(int count);

    // ----- Omniscience methods ----- //

    int currentXTile();
    int currentYTile();
    char currentDirection();

    double currentXPosMeters();
    double currentYPosMeters();
    double currentRotationDegrees();

private:
    const Maze* m_maze;
    Mouse* m_mouse;
    MazeGraphic* m_mazeGraphic;
    std::set<char> m_allowableTileTextCharacters;
    MouseInterfaceOptions m_options;

    // Cache of tiles, for making clearAll methods faster
    std::set<std::pair<int, int>> m_tilesWithColor;
    std::set<std::pair<int, int>> m_tilesWithText;

    void ensureDiscreteInterface(const std::string& callingFunction) const;
    void ensureContinuousInterface(const std::string& callingFunction) const;
    void ensureAllowOmniscience(const std::string& callingFunction) const;

    bool isWall(std::pair<int, int> position, Direction direction);
    bool hasOpposingWall(int x, int y, Direction direction) const;
    std::pair<std::pair<int, int>, Direction> getOpposingWall(int x, int y, Direction direction) const;
};

} // namespace sim
