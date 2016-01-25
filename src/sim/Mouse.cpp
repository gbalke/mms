#include "Mouse.h"

#include "units/Meters.h"
#include "units/MetersPerSecond.h"
#include "units/Polar.h"

#include "Assert.h"
#include "ContainerUtilities.h"
#include "CPMath.h"
#include "Directory.h"
#include "GeometryUtilities.h"
#include "MouseParser.h"
#include "Param.h"
#include "State.h"

namespace sim {

Mouse::Mouse(const Maze* maze) : m_maze(maze), m_currentGyro(RadiansPerSecond(0.0)) {
}

bool Mouse::initialize(
        const std::string& mouseFile,
        Direction initialDirection) {

    // We begin with the assumption that the initialization will succeed
    bool success = true;

    // The initial translation of the mouse is just the center of the starting tile
    Meters halfOfTileDistance = Meters((P()->wallLength() + P()->wallWidth()) / 2.0);
    m_initialTranslation = Cartesian(halfOfTileDistance, halfOfTileDistance);
    m_currentTranslation = m_initialTranslation;

    // The initial rotation of the mouse, however, is determined by the options
    m_initialRotation = DIRECTION_TO_ANGLE.at(initialDirection);
    m_currentRotation = m_initialRotation;

    // Create the mouse parser object
    MouseParser parser(Directory::getResMouseDirectory() + mouseFile, &success);
    if (!success) { // A checkpoint so that we can fail faster
        return false;
    }

    // Initialize the body, wheels, and sensors, such that they have the
    // correct initial translation and rotation
    m_initialBodyPolygon = parser.getBody(m_initialTranslation, m_initialRotation, &success);
    m_wheels = parser.getWheels(m_initialTranslation, m_initialRotation, &success);
    m_sensors = parser.getSensors(m_initialTranslation, m_initialRotation, *m_maze, &success);

    // Initialize the wheel speed adjustment factors based on the wheels
    m_wheelSpeedAdjustmentFactors = getWheelSpeedAdjustmentFactors(
        m_initialTranslation,
        m_initialRotation,
        m_wheels);

    // Initialize the curve turn factors, based on previously determined info
    m_curveTurnFactors = getCurveTurnFactors(
        m_initialTranslation,
        m_initialRotation,
        m_wheels,
        m_wheelSpeedAdjustmentFactors,
        Meters(P()->wallLength() / 2.0) * 0.5 * M_PI);

    // Initialize the collision polygon; this is technically not correct since
    // we should be using union, not convexHull, but it's a good approximation
    std::vector<Polygon> polygons;
    polygons.push_back(m_initialBodyPolygon);
    for (std::pair<std::string, Wheel> pair : m_wheels) {
        polygons.push_back(pair.second.getInitialPolygon());
    }
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        polygons.push_back(pair.second.getInitialPolygon());
    }
    m_initialCollisionPolygon = GeometryUtilities::convexHull(polygons);

    // Initialize the center of mass polygon
    m_initialCenterOfMassPolygon = GeometryUtilities::createCirclePolygon(m_initialTranslation, Meters(.005), 8);

    // Return success
    return success;
}

Cartesian Mouse::getInitialTranslation() const {
    return m_initialTranslation;
}

Radians Mouse::getInitialRotation() const {
    return m_initialRotation;
}

Cartesian Mouse::getCurrentTranslation() const {
    return m_currentTranslation;
}

Radians Mouse::getCurrentRotation() const {
    return m_currentRotation;
}

std::pair<int, int> Mouse::getCurrentDiscretizedTranslation() const {
    static Meters tileLength = Meters(P()->wallLength() + P()->wallWidth());
    Cartesian currentTranslation = getCurrentTranslation();
    int x = static_cast<int>(
        std::floor(
            currentTranslation.getX() / tileLength
        )
    );
    int y = static_cast<int>(
        std::floor(
            currentTranslation.getY() / tileLength
        )
    );
    return std::make_pair(x, y);
}

Direction Mouse::getCurrentDiscretizedRotation() const {
    int dir = static_cast<int>(
        std::floor(
            (getCurrentRotation() + Degrees(45)).getRadiansZeroTo2pi() /
            Degrees(90).getRadiansZeroTo2pi()
        )
    );
    switch (dir) {
        case 0:
            return Direction::EAST;
        case 1:
            return Direction::NORTH;
        case 2:
            return Direction::WEST;
        case 3:
            return Direction::SOUTH;
    }
}

void Mouse::teleport(const Coordinate& translation, const Angle& rotation) {
    m_currentTranslation = translation;
    m_currentRotation = rotation;
}

Polygon Mouse::getCurrentBodyPolygon(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    return getCurrentPolygon(m_initialBodyPolygon, currentTranslation, currentRotation);
}

Polygon Mouse::getCurrentCollisionPolygon(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    return getCurrentPolygon(m_initialCollisionPolygon, currentTranslation, currentRotation);
}

Polygon Mouse::getCurrentCenterOfMassPolygon(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    return getCurrentPolygon(m_initialCenterOfMassPolygon, currentTranslation, currentRotation);
}

std::vector<Polygon> Mouse::getCurrentWheelPolygons(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    std::vector<Polygon> polygons;
    for (std::pair<std::string, Wheel> pair : m_wheels) {
        polygons.push_back(getCurrentPolygon(pair.second.getInitialPolygon(), currentTranslation, currentRotation));
    }
    return polygons;
}

std::vector<Polygon> Mouse::getCurrentWheelSpeedIndicatorPolygons(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    std::vector<Polygon> polygons;
    for (std::pair<std::string, Wheel> pair : m_wheels) {
        polygons.push_back(getCurrentPolygon(pair.second.getSpeedIndicatorPolygon(), currentTranslation, currentRotation));
    }
    return polygons;
}

std::vector<Polygon> Mouse::getCurrentSensorPolygons(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    std::vector<Polygon> polygons;
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        polygons.push_back(getCurrentPolygon(pair.second.getInitialPolygon(), currentTranslation, currentRotation));
    }
    return polygons;
}

std::vector<Polygon> Mouse::getCurrentSensorViewPolygons(
        const Coordinate& currentTranslation, const Angle& currentRotation) const {
    std::vector<Polygon> polygons;
    for (std::pair<std::string, Sensor> pair : m_sensors) {
        std::pair<Cartesian, Radians> translationAndRotation =
            getCurrentSensorPositionAndDirection(
                pair.second,
                currentTranslation,
                currentRotation);
        polygons.push_back(
            pair.second.getCurrentViewPolygon(
                translationAndRotation.first,
                translationAndRotation.second,
                *m_maze));
    }
    return polygons;
}

void Mouse::update(const Duration& elapsed) {

    // NOTE: This is a *very* performance critical function

    m_updateMutex.lock();

    MetersPerSecond sumDx(0);
    MetersPerSecond sumDy(0);
    RadiansPerSecond sumDr(0);

    for (const std::pair<std::string, Wheel>& wheel : m_wheels) {

        m_wheels.at(wheel.first).updateRotation(wheel.second.getAngularVelocity() * elapsed);

        std::pair<MetersPerSecond, RadiansPerSecond> ratesOfChange = getRatesOfChange(
            getInitialTranslation(),
            getInitialRotation(),
            wheel.second.getInitialPosition(),
            wheel.second.getInitialDirection(),
            (
                wheel.second.getAngularVelocity() *
                wheel.second.getRadius()
            )
        );

        sumDx += ratesOfChange.first * getCurrentRotation().getCos();
        sumDy += ratesOfChange.first * getCurrentRotation().getSin();
        sumDr += ratesOfChange.second;
    }

    MetersPerSecond aveDx = sumDx / static_cast<double>(m_wheels.size());
    MetersPerSecond aveDy = sumDy / static_cast<double>(m_wheels.size());
    RadiansPerSecond aveDr = sumDr / static_cast<double>(m_wheels.size());

    m_currentGyro = aveDr;
    m_currentRotation += Radians(aveDr * elapsed);
    m_currentTranslation += Cartesian(aveDx * elapsed, aveDy * elapsed);

    for (std::pair<std::string, Sensor> pair : m_sensors) {
        std::pair<Cartesian, Radians> translationAndRotation =
            getCurrentSensorPositionAndDirection(
                pair.second,
                m_currentTranslation,
                m_currentRotation);
        m_sensors.at(pair.first).updateReading(
            translationAndRotation.first,
            translationAndRotation.second,
            *m_maze);
    }

    m_elapsedSimTime += elapsed;

    m_updateMutex.unlock();
}

bool Mouse::hasWheel(const std::string& name) const {
    return ContainerUtilities::mapContains(m_wheels, name);
}

RadiansPerSecond Mouse::getWheelMaxSpeed(const std::string& name) const {
    ASSERT_TR(ContainerUtilities::mapContains(m_wheels, name));
    return m_wheels.at(name).getMaxAngularVelocityMagnitude();
}

void Mouse::setWheelSpeeds(const std::map<std::string, RadiansPerSecond>& wheelSpeeds) {
    m_updateMutex.lock();
    for (std::pair<std::string, RadiansPerSecond> pair : wheelSpeeds) {
        ASSERT_TR(ContainerUtilities::mapContains(m_wheels, pair.first));
        ASSERT_LE(
            std::abs(pair.second.getRevolutionsPerMinute()),
            getWheelMaxSpeed(pair.first).getRevolutionsPerMinute());
        m_wheels.at(pair.first).setAngularVelocity(pair.second);
    }
    m_updateMutex.unlock();
}

void Mouse::setWheelSpeedsForMoveForward(double fractionOfMaxSpeed) {
    setWheelSpeedsForMovement(fractionOfMaxSpeed, 1.0, 0.0);
}

void Mouse::setWheelSpeedsForTurnLeft(double fractionOfMaxSpeed) {
    setWheelSpeedsForMovement(fractionOfMaxSpeed, 0.0, 1.0);
}

void Mouse::setWheelSpeedsForTurnRight(double fractionOfMaxSpeed) {
    setWheelSpeedsForMovement(fractionOfMaxSpeed, 0.0, -1.0);
}

void Mouse::setWheelSpeedsForCurveTurnLeft(double fractionOfMaxSpeed) {
    setWheelSpeedsForMovement(fractionOfMaxSpeed, m_curveTurnFactors.first, m_curveTurnFactors.second);
}

void Mouse::setWheelSpeedsForCurveTurnRight(double fractionOfMaxSpeed) {
    setWheelSpeedsForMovement(fractionOfMaxSpeed, m_curveTurnFactors.first, -1 * m_curveTurnFactors.second);
}

void Mouse::stopAllWheels() {
    std::map<std::string, RadiansPerSecond> wheelSpeeds;
    for (std::pair<std::string, Wheel> wheel : m_wheels) {
        wheelSpeeds.insert(std::make_pair(wheel.first, RadiansPerSecond(0)));
    }
    setWheelSpeeds(wheelSpeeds);
}

EncoderType Mouse::getWheelEncoderType(const std::string& name) const {
    ASSERT_TR(hasWheel(name));
    return m_wheels.at(name).getEncoderType();
}

double Mouse::getWheelEncoderTicksPerRevolution(const std::string& name) const {
    ASSERT_TR(hasWheel(name));
    return m_wheels.at(name).getEncoderTicksPerRevolution();
}

int Mouse::readWheelAbsoluteEncoder(const std::string& name) const {
    ASSERT_TR(hasWheel(name));
    m_updateMutex.lock();
    int encoderReading = m_wheels.at(name).readAbsoluteEncoder();
    m_updateMutex.unlock();
    return encoderReading;
}

int Mouse::readWheelRelativeEncoder(const std::string& name) const {
    ASSERT_TR(hasWheel(name));
    m_updateMutex.lock();
    int encoderReading = m_wheels.at(name).readRelativeEncoder();
    m_updateMutex.unlock();
    return encoderReading;
}

void Mouse::resetWheelRelativeEncoder(const std::string& name) {
    ASSERT_TR(hasWheel(name));
    m_updateMutex.lock();
    m_wheels.at(name).resetRelativeEncoder();
    m_updateMutex.unlock();
}

bool Mouse::hasSensor(const std::string& name) const {
    return ContainerUtilities::mapContains(m_sensors, name);
}

double Mouse::readSensor(const std::string& name) const {
    ASSERT_TR(hasSensor(name));
    return m_sensors.at(name).read();
}

Seconds Mouse::getSensorReadDuration(const std::string& name) const {
    ASSERT_TR(ContainerUtilities::mapContains(m_sensors, name));
    return m_sensors.at(name).getReadDuration();
}

RadiansPerSecond Mouse::readGyro() const {
    return m_currentGyro;
}

Seconds Mouse::getElapsedSimTime() const {
    m_updateMutex.lock();
    Seconds elapsedSimTime = m_elapsedSimTime;
    m_updateMutex.unlock();
    return elapsedSimTime;
}

Polygon Mouse::getCurrentPolygon(const Polygon& initialPolygon,
        const Cartesian& currentTranslation, const Radians& currentRotation) const {
    return initialPolygon
        .translate(currentTranslation - getInitialTranslation())
        .rotateAroundPoint(currentRotation - getInitialRotation(), currentTranslation);
}

std::pair<Cartesian, Radians> Mouse::getCurrentSensorPositionAndDirection(
        const Sensor& sensor,
        const Cartesian& currentTranslation,
        const Radians& currentRotation) const {
    Cartesian translationDelta = currentTranslation - getInitialTranslation();
    Radians rotationDelta = currentRotation - getInitialRotation();
    return std::make_pair(
        GeometryUtilities::rotateVertexAroundPoint(
            GeometryUtilities::translateVertex(
                sensor.getInitialPosition(),
                translationDelta),
            rotationDelta,
            currentTranslation),
        sensor.getInitialDirection() + rotationDelta
    );
}

void Mouse::setWheelSpeedsForMovement(double fractionOfMaxSpeed, double forwardFactor, double turnFactor) {

    // We can think about setting the wheels speeds for particular movements as
    // a linear combination of the forward movement and the turn movement. For
    // instance, the (normalized) linear combination of the forward and turn
    // components for moving forward is just 1.0 and 0.0, respectively. For
    // turning left, it's 0.0 and 1.0, respectively, and for turning right it's
    // 0.0 and -1.0, respectively. For curve turns, it's some other linear
    // combination. Note that we normalize here since we don't know anything
    // about the wheel speeds for a particular component. Thus, we must ensure
    // that the sum of the magnitudes of the components is in [0.0, 1.0] so
    // that we don't try to set any wheel speeds greater than the max.

    // First we normalize the factors so that the sum of the magnitudes is in [0.0, 1.0]
    double factorMagnitude = std::abs(forwardFactor) + std::abs(turnFactor);
    double normalizedForwardFactor = forwardFactor / factorMagnitude;
    double normalizedTurnFactor = turnFactor / factorMagnitude;

    // Now we just double check that the magnitudes are where we expect them to be
    double normalizedFactorMagnitude = std::abs(normalizedForwardFactor) + std::abs(normalizedTurnFactor);
    ASSERT_LE(0.0, normalizedFactorMagnitude);
    ASSERT_LE(normalizedFactorMagnitude, 1.0);

    // Now set the wheel speeds based on the normalized factors
    std::map<std::string, RadiansPerSecond> wheelSpeeds;
    for (std::pair<std::string, Wheel> wheel : m_wheels) {
        ASSERT_TR(ContainerUtilities::mapContains(m_wheelSpeedAdjustmentFactors, wheel.first));
        std::pair<double, double> adjustmentFactors = m_wheelSpeedAdjustmentFactors.at(wheel.first);
        wheelSpeeds.insert(
            std::make_pair(
                wheel.first,
                (
                    wheel.second.getMaxAngularVelocityMagnitude() *
                    fractionOfMaxSpeed *
                    (
                        normalizedForwardFactor * adjustmentFactors.first +
                        normalizedTurnFactor * adjustmentFactors.second
                    )
                )
            )
        );
    }
    setWheelSpeeds(wheelSpeeds);
}

std::map<std::string, std::pair<double, double>> Mouse::getWheelSpeedAdjustmentFactors(
        const Cartesian& initialTranslation,
        const Radians& initialRotation,
        const std::map<std::string, Wheel>& wheels) const {

    // Right now, the heueristic that we're using is that if a wheel greatly
    // contributes to moving forward or turning, then its adjustment factors
    // should be high for moving forward or turning, respectively. That is, if
    // we've got a wheel that's facing to the right, we don't want to turn that
    // wheel when we're trying to move forward. Instead, we should only turn
    // the wheels that will actually contribute to the forward movement of the
    // mouse. I'm not yet sure if we should take wheel size and/or max angular
    // velocity magnitude into account, but I've done so here.

    // First, construct the rates of change pairs
    std::map<std::string, std::pair<MetersPerSecond, RadiansPerSecond>> ratesOfChangePairs;
    for (std::pair<std::string, Wheel> wheel : wheels) {
        ratesOfChangePairs.insert(
            std::make_pair(
                wheel.first,
                getRatesOfChange(
                    initialTranslation,
                    initialRotation,
                    wheel.second.getInitialPosition(),
                    wheel.second.getInitialDirection(),
                    (
                        wheel.second.getMaxAngularVelocityMagnitude() *
                        wheel.second.getRadius()
                    )
                )
            )
        );
    }

    // Then determine the largest magnitude
    MetersPerSecond maxForwardRateOfChangeMagnitude(0);
    RadiansPerSecond maxRadialRateOfChangeMagnitude(0);
    for (const std::pair<std::string, std::pair<MetersPerSecond, RadiansPerSecond>>& pair : ratesOfChangePairs) {
        MetersPerSecond forwardRateOfChangeMagnitude(std::abs(pair.second.first.getMetersPerSecond()));
        RadiansPerSecond radialRateOfChangeMagnitude(std::abs(pair.second.second.getRadiansPerSecond()));
        if (maxForwardRateOfChangeMagnitude < forwardRateOfChangeMagnitude) {
            maxForwardRateOfChangeMagnitude = forwardRateOfChangeMagnitude;
        }
        if (maxRadialRateOfChangeMagnitude < radialRateOfChangeMagnitude) {
            maxRadialRateOfChangeMagnitude = radialRateOfChangeMagnitude;
        }
    }

    // Then divide by the largest magnitude, ensuring values in [-1.0, 1.0]
    std::map<std::string, std::pair<double, double>> adjustmentFactors;
    for (std::pair<std::string, std::pair<MetersPerSecond, RadiansPerSecond>> pair : ratesOfChangePairs) {
        double normalizedForwardContribution = pair.second.first / maxForwardRateOfChangeMagnitude;
        double normalizedRadialContribution = pair.second.second / maxRadialRateOfChangeMagnitude;
        ASSERT_LE(-1.0, normalizedForwardContribution);
        ASSERT_LE(-1.0, normalizedRadialContribution);
        ASSERT_LE(normalizedForwardContribution, 1.0);
        ASSERT_LE(normalizedRadialContribution, 1.0);
        adjustmentFactors.insert(
            std::make_pair(
                pair.first,
                std::make_pair(
                    normalizedForwardContribution,
                    normalizedRadialContribution)));
    }
    
    return adjustmentFactors;
}

std::pair<double, double> Mouse::getCurveTurnFactors(
        const Cartesian& initialTranslation,
        const Radians& initialRotation,
        const std::map<std::string, Wheel>& wheels,
        const std::map<std::string, std::pair<double, double>> wheelSpeedAdjustmentFactors,
        const Meters& curveTurnArcLength) const {

    // Determine the total forward and turn rate of change from all wheels
    MetersPerSecond totalForwardRateOfChange(0);
    RadiansPerSecond totalRadialRateOfChange(0);
    for (const std::pair<std::string, Wheel>& wheel : wheels) {

        // The maximum linear velocity of the wheel
        MetersPerSecond maxLinearVelocity =
            wheel.second.getMaxAngularVelocityMagnitude() *
            wheel.second.getRadius();

        // For each of the wheel speed adjustment factors, calculate the wheel's
        // contributions. Remember that each of these factors corresponds to
        // the fraction of the max wheel speed such that the mouse performs a
        // particular movement (moving forward or turning) most optimally.
        ASSERT_TR(ContainerUtilities::mapContains(wheelSpeedAdjustmentFactors, wheel.first));
        std::pair<double, double> adjustmentFactors = wheelSpeedAdjustmentFactors.at(wheel.first);
        for (double adjustmentFactor : {adjustmentFactors.first, adjustmentFactors.second}) {

            std::pair<MetersPerSecond, RadiansPerSecond> ratesOfChange = getRatesOfChange(
                initialTranslation,
                initialRotation,
                wheel.second.getInitialPosition(),
                wheel.second.getInitialDirection(),
                maxLinearVelocity * adjustmentFactor
            );

            totalForwardRateOfChange += ratesOfChange.first;
            totalRadialRateOfChange += ratesOfChange.second;
        }
    }

    // The main idea here is that, for a curve turn, we want the mouse to move
    // forward a distance equal to the length of the arc we'd like it to travel
    Meters totalDistance = curveTurnArcLength;
    Degrees totalRotation = Degrees(90);

    // We want to return a pair of factors, A and B, such that:
    //
    //  totalForwardRateOfChange * A   totalDistance
    //  ---------------------------- = -------------
    //  totalRadialRateOfChange  * B   totalRotation
    //
    // That is, we'd like to return two numbers, A and B, such that they
    // appropriately scale the forward and turn contributions of the wheels so
    // that the mouse travels the distance of the curve turn arc in the exact
    // same amount of time that it rotates ninty degrees. Thus we have that:
    //
    //      totalDistance   totalRadialRateOfChange
    //  A = ------------- * ------------------------ * B
    //      totalRotation   totalForwardRateOfChange
    //
    // Then we can just choose B = 1.0 and solve for A

    double B = 1.0;
    double A =
        (totalDistance.getMeters() / totalRotation.getRadiansZeroTo2pi()) *
        (totalRadialRateOfChange.getRadiansPerSecond() / totalForwardRateOfChange.getMetersPerSecond());

    return std::make_pair(A, B);
}

std::pair<MetersPerSecond, RadiansPerSecond> Mouse::getRatesOfChange(
        const Cartesian& initialTranslation,
        const Radians& initialRotation,
        const Cartesian& wheelInitialPosition,
        const Radians& wheelInitialDirection,
        const MetersPerSecond& wheelLinearVelocity) const {

    MetersPerSecond forwardRateOfChange = MetersPerSecond(
        wheelLinearVelocity.getMetersPerSecond() * 
        (initialRotation - wheelInitialDirection).getCos()
    );

    Cartesian wheelToCenter = initialTranslation - wheelInitialPosition;
    RadiansPerSecond radialRateOfChange = RadiansPerSecond(
        wheelLinearVelocity.getMetersPerSecond() *
        (wheelToCenter.getTheta() - wheelInitialDirection).getSin() *
        (1.0 / wheelToCenter.getRho().getMeters())
    );

    return std::make_pair(forwardRateOfChange, radialRateOfChange);
}
    

} // namespace sim
