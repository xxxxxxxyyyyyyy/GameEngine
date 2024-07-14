#include "math_types.h"

/**
 * @brief Creates and returns a new transform, using a zero
 * vector for position, identity quaternion for rotation, and
 * a one vector for scale. Also has a null parent. Marked dirty
 * by default.
 */
API transform transform_create(void);

/**
 * @brief Creates a transform from the given position.
 * Uses a zero rotation and a one scale.
 *
 * @param position The position to be used.
 * @return A new transform.
 */
API transform transform_from_position(vec3 position);

/**
 * @brief Creates a transform from the given rotation.
 * Uses a zero position and a one scale.
 *
 * @param rotation The rotation to be used.
 * @return A new transform.
 */
API transform transform_from_rotation(quaterion rotation);

/**
 * @brief Creates a transform from the given position and rotation.
 * Uses a one scale.
 *
 * @param position The position to be used.
 * @param rotation The rotation to be used.
 * @return A new transform.
 */
API transform transform_from_position_rotation(vec3 position, quaterion rotation);

/**
 * @brief Creates a transform from the given position, rotation and scale.
 *
 * @param position The position to be used.
 * @param rotation The rotation to be used.
 * @param scale The scale to be used.
 * @return A new transform.
 */
API transform transform_from_position_rotation_scale(vec3 position, quaterion rotation, vec3 scale);

/**
 * @brief Returns a pointer to the provided transform's parent.
 *
 * @param t A pointer to the transform whose parent to retrieve.
 * @return A pointer to the parent transform.
 */
API transform* transform_parent_get(transform* t);

/**
 * @brief Sets the parent of the provided transform.
 *
 * @param t A pointer to the transform whose parent will be set.
 * @param parent A pointer to the parent transform.
 */
API void transform_parent_set(transform* t, transform* parent);

/**
 * @brief Returns the position of the given transform.
 *
 * @param t A constant pointer whose position to get.
 * @return A copy of the position.
 */
API vec3 transform_position_get(const transform* t);

/**
 * @brief Sets the position of the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param position The position to be set.
 */
API void transform_position_set(transform* t, vec3 position);

/**
 * @brief Applies a translation to the given transform. Not the
 * same as setting.
 *
 * @param t A pointer to the transform to be updated.
 * @param translation The translation to be applied.
 */
API void transform_translate(transform* t, vec3 translation);

/**
 * @brief Returns the rotation of the given transform.
 *
 * @param t A constant pointer whose rotation to get.
 * @return A copy of the rotation.
 */
API quaterion transform_rotation_get(const transform* t);

/**
 * @brief Sets the rotation of the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param rotation The rotation to be set.
 */
API void transform_rotation_set(transform* t, quaterion rotation);

/**
 * @brief Applies a rotation to the given transform. Not the
 * same as setting.
 *
 * @param t A pointer to the transform to be updated.
 * @param rotation The rotation to be applied.
 */
API void transform_rotate(transform* t, quaterion rotation);

/**
 * @brief Returns the scale of the given transform.
 *
 * @param t A constant pointer whose scale to get.
 * @return A copy of the scale.
 */
API vec3 transform_scale_get(const transform* t);

/**
 * @brief Sets the scale of the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param scale The scale to be set.
 */
API void transform_scale_set(transform* t, vec3 scale);

/**
 * @brief Applies a scale to the given transform. Not the
 * same as setting.
 *
 * @param t A pointer to the transform to be updated.
 * @param scale The scale to be applied.
 */
API void transform_scale(transform* t, vec3 scale);

/**
 * @brief Sets the position and rotation of the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param position The position to be set.
 * @param rotation The rotation to be set.
 */
API void transform_position_rotation_set(transform* t, vec3 position, quaterion rotation);

/**
 * @brief Sets the position, rotation and scale of the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param position The position to be set.
 * @param rotation The rotation to be set.
 * @param scale The scale to be set.
 */
API void transform_position_rotation_scale_set(transform* t, vec3 position, quaterion rotation, vec3 scale);

/**
 * @brief Applies translation and rotation to the given transform.
 *
 * @param t A pointer to the transform to be updated.
 * @param translation The translation to be applied.
 * @param rotation The rotation to be applied.
 * @return API
 */
API void transform_translate_rotate(transform* t, vec3 translation, quaterion rotation);

/**
 * @brief Retrieves the local transformation matrix from the provided transform.
 * Automatically recalculates the matrix if it is dirty. Otherwise, the already
 * calculated one is returned.
 *
 * @param t A pointer to the transform whose matrix to retrieve.
 * @return A copy of the local transformation matrix.
 */
API matrix4 transform_local_get(transform* t);

/**
 * @brief Obtains the world matrix of the given transform
 * by examining its parent (if there is one) and multiplying it
 * against the local matrix.
 *
 * @param t A pointer to the transform whose world matrix to retrieve.
 * @return A copy of the world matrix.
 */
API matrix4 transform_world_get(transform* t);