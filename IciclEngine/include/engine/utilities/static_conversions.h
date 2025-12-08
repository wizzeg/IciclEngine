#pragma once
#include <glm/glm.hpp>
#include <assimp/matrix4x4.h>

struct conversions
{
	static glm::mat4 ai_matrix4x4_to_glm_mat4(const aiMatrix4x4& ai_matrix) {
		glm::mat4 glm_matrix = glm::mat4(0);
		glm_matrix[0][0] = ai_matrix.a1; glm_matrix[1][0] = ai_matrix.b1; glm_matrix[2][0] = ai_matrix.c1; glm_matrix[3][0] = ai_matrix.d1;
		glm_matrix[0][1] = ai_matrix.a2; glm_matrix[1][1] = ai_matrix.b2; glm_matrix[2][1] = ai_matrix.c2; glm_matrix[3][1] = ai_matrix.d2;
		glm_matrix[0][2] = ai_matrix.a3; glm_matrix[1][2] = ai_matrix.b3; glm_matrix[2][2] = ai_matrix.c3; glm_matrix[3][2] = ai_matrix.d3;
		glm_matrix[0][3] = ai_matrix.a4; glm_matrix[1][3] = ai_matrix.b4; glm_matrix[2][3] = ai_matrix.c4; glm_matrix[3][3] = ai_matrix.d4;
		return glm_matrix;
	}
};
