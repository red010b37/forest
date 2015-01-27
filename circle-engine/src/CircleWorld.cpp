// Circle Engine (c) 2015 Micah Elizabeth Scott
// MIT license

#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "CircleWorld.h"
#include <stdio.h>

using namespace ci;
using namespace std;


CircleWorld::ExcNodeNotFound::ExcNodeNotFound(const string &name) throw()
{
    snprintf(mMessage, sizeof mMessage, "Could not locate required SVG node: %s", name.c_str());
}

const svg::Node& CircleWorld::findNode(const string &name)
{
    const svg::Node *node = mSvg->findNode(name);
    if (!node)
        throw ExcNodeNotFound(name);
    return *node;
}

Shape2d CircleWorld::findShape(const string &name)
{
    return findNode(name).getShapeAbsolute();
}

Vec2f CircleWorld::findMetric(const string &name)
{
    Vec2f v = findNode(name).getBoundingBoxAbsolute().getSize();
    printf("[metric] %s = (%f, %f)\n", name.c_str(), v.x, v.y);
    return v;
}

b2Vec2 CircleWorld::vecToBox(Vec2f v)
{
    return b2Vec2( v.x * kMetersPerPoint, v.y * kMetersPerPoint );
}

Vec2f CircleWorld::vecFromBox(b2Vec2 v)
{
    return Vec2f( v.x / kMetersPerPoint, v.y / kMetersPerPoint );
}

void CircleWorld::setup(svg::DocRef doc, ci::ImageSourceRef colorTable)
{
    mSvg = doc;
    mColorTable = colorTable;

    b2Vec2 gravity( 0.0f, vecToBox(findMetric("gravity")).y );
    mB2World = new b2World(gravity);

    Vec2f metricParticleRadius = findMetric("particle-radius");
    mTriangulatePrecision = 1.0f / metricParticleRadius.y;
    
    b2ParticleSystemDef particleSystemDef;
    particleSystemDef.colorMixingStrength = 0.01;
    mParticleSystem = mB2World->CreateParticleSystem(&particleSystemDef);
    mParticleSystem->SetGravityScale(0.4f);
    mParticleSystem->SetDensity(1.2f);
    mParticleSystem->SetRadius(vecToBox(metricParticleRadius).y / 2);
    mParticleSystem->SetDestructionByAge(true);
    
    setupObstacles(findShape("obstacles"));
    setupStrands(findShape("strands"));

    for (int index = 0;; index++) {
        char name[64];
        snprintf(name, sizeof name, "spinner-%d", index);
        const svg::Node* node = mSvg->findNode(name);
        if (node) {
            setupSpinner(node->getShape());
        } else {
            break;
        }
    }
}

void CircleWorld::setupObstacles(const Shape2d& shape)
{
    // Store a triangulated mesh, for drawing quickly
    Triangulator triangulator(shape, mTriangulatePrecision);
    mObstacles = triangulator.calcMesh();
    
    b2BodyDef groundBodyDef;
    b2Body *ground = mB2World->CreateBody(&groundBodyDef);
    addFixturesForMesh(ground, mObstacles, 0.0f);
}

void CircleWorld::setupStrands(const Shape2d& shape)
{
    mOriginPoints.clear();
    
    mForceGridExtent = shape.calcBoundingBox();
    mForceGridResolution = findMetric("force-grid-resolution").y;
    mForceGridStrength = findMetric("force-grid-strength").y * 1e-3;
    mForceGridWidth = mForceGridExtent.getWidth() / mForceGridResolution;
    mForceGrid.resize(mForceGridWidth * int(mForceGridExtent.getHeight() / mForceGridResolution));
    
    for (unsigned contour = 0; contour < shape.getNumContours(); contour++) {
        const Path2d &path = shape.getContour(contour);
        unsigned steps = path.calcLength() / mForceGridResolution * 4.0;

        mOriginPoints.push_back(path.getPosition(0.0f));

        for (unsigned step = 0; step < steps; step++) {
            float t = step / float(steps - 1);
            Vec2f position = path.getPosition(t);
            Vec2f tangent = path.getTangent(t);
            unsigned x = (position.x - mForceGridExtent.getX1()) / mForceGridResolution;
            unsigned y = (position.y - mForceGridExtent.getY1()) / mForceGridResolution;
            unsigned idx = x + y * mForceGridWidth;
            if (x < mForceGridWidth && idx < mForceGrid.size()) {
                mForceGrid[idx] += tangent;
            }
        }
    }

    // Bounding rectangle of all origin points
    mOriginBounds = mOriginPoints;
}

void CircleWorld::setupSpinner(const ci::Shape2d& shape)
{
    Vec2f center = shape.calcPreciseBoundingBox().getCenter();

    Triangulator triangulator(shape.transformCopy(MatrixAffine2f::makeTranslate(-center)),
                              mTriangulatePrecision);
    TriMesh2d mesh = triangulator.calcMesh();
    mSpinnerMeshes.push_back(mesh);
    
    b2BodyDef bodyDef;
    bodyDef.position = vecToBox(center);

    b2Body *obj = mB2World->CreateBody(&bodyDef);
    mSpinnerBodies.push_back(obj);
    addFixturesForMesh(obj, mesh, 0.0f);
}

void CircleWorld::addFixturesForMesh(b2Body *body, ci::TriMesh2d &mesh, float density)
{
    b2PolygonShape polygon;
    for (size_t tri = 0; tri < mesh.getNumTriangles(); tri++) {
        Vec2f v[3];
        mesh.getTriangleVertices(tri, &v[0], &v[1], &v[2]);
        float area = 0.5f * (v[2] - v[0]).cross(v[1] - v[0]);
        if (area > kMinTriangleArea) {
            b2Vec2 bv[3];
            for (unsigned i = 0; i < 3; i++) {
                bv[i] = vecToBox(v[i]);
            }
            polygon.Set(&bv[0], 3);
            body->CreateFixture(&polygon, density);
        }
    }
}

void CircleWorld::update()
{
    updateSpinners();

    for (unsigned i = 0; i < 20; i++) {
        newParticle();
    }

    applyGridForces();

    mStepNumber++;
    mB2World->Step( 1 / 60.0f, 1, 1, 2 );
}

void CircleWorld::updateSpinners()
{
    Perlin p(4, 0);
    for (unsigned i = 0; i < mSpinnerBodies.size(); i++) {
        b2Body *spinner = mSpinnerBodies[i];
        float angle = p.fBm(mStepNumber * 0.002, i) * 50.0;
        spinner->SetTransform(spinner->GetPosition(), angle);
    }
    
}

void CircleWorld::applyGridForces()
{
    b2Vec2* positions = mParticleSystem->GetPositionBuffer();
    b2Vec2* velocities = mParticleSystem->GetVelocityBuffer();
    unsigned numParticles = mParticleSystem->GetParticleCount();
    float scale = 1.0f / (mForceGridResolution * kMetersPerPoint);
    b2Vec2 gridCorner = vecToBox(mForceGridExtent.getUpperLeft());
    
    for (unsigned i = 0; i < numParticles; i++) {
        b2Vec2 pos = (positions[i] - gridCorner) * scale;
        int ix = pos.x;
        unsigned idx = ix + int(pos.y) * mForceGridWidth;
        if (ix < mForceGridWidth && idx < mForceGrid.size()) {
            velocities[i] += vecToBox(mForceGrid[idx]) * mForceGridStrength;
        }
    }
}

void CircleWorld::newParticle()
{
    b2ParticleDef pd;
    Vec2f pos = mOriginPoints[Rand::randInt(mOriginPoints.size())];

    float x = (pos.x - mOriginBounds.getX1()) / mOriginBounds.getWidth();
    float y = 0.5 + 0.49 * sinf(mStepNumber * 0.001);
    Vec2i loc( x * (mColorTable.getWidth() - 1), y * (mColorTable.getHeight() - 1) );
    auto pix = mColorTable.getPixel(loc);

    pd.position = vecToBox(pos);
    pd.flags = b2_colorMixingParticle | b2_tensileParticle;
    pd.lifetime = 20.0f;
    pd.color.Set(pix.r, pix.g, pix.b, pix.a);

    mParticleSystem->CreateParticle(pd);
}