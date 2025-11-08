struct RayPayload {
  vec3 color;
  vec3 origin;
  vec3 normal;
  uint hitInfo;
};

float RandomValue(inout uint state)
{
    //pcg hash
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    result = (result >> 22u) ^ result;
    return result / 4294967295.0;
}

vec2 RandomCircle(inout uint state) {
    float radius = sqrt(RandomValue(state));
    float angle = 6.2831853 * RandomValue(state);
    return radius * vec2(cos(angle), sin(angle));
}

vec3 RandomSphere(inout uint state) {
    vec2 disk = RandomCircle(state);
    float norm = dot(disk, disk);
    float radius = 2.0 * sqrt(1.0 - norm);
    float z = 1.0 - 2.0 * norm;
    return vec3(radius * disk, z);
}