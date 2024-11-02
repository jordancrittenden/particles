#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Generate vertices for a circle with radius r2
std::vector<float> generateCircleVerticesUnrolled(float r2, int segments) {
    std::vector<float> vertices;
    float thetaStep = 2.0f * M_PI / segments;
    for (int i = 0; i < segments; ++i) {
        float theta = i * thetaStep;
        vertices.push_back(r2 * cos(theta));
        vertices.push_back(r2 * sin(theta));
    }
    return vertices;
}

// Set up transformation matrix for each circle
glm::mat4 getCircleModelMatrix(float angle, float r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}