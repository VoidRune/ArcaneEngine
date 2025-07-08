bool RayBox(vec3 rayOrigin, vec3 rayDirection, vec3 p1, vec3 p2, out float tmin, out float tmax)
{
    vec3 dirFrac = 1 / rayDirection;
    vec3 t1 = (p1 - rayOrigin) * dirFrac;
    vec3 t2 = (p2 - rayOrigin) * dirFrac;
    tmin = max(max(min(t1.x, t2.x), min(t1.y, t2.y)), min(t1.z, t2.z));
    tmax = min(min(max(t1.x, t2.x), max(t1.y, t2.y)), max(t1.z, t2.z));
    return tmax >= tmin && tmax >= 0.0;
}

float max3(vec3 v) {
    return max(max(v.x, v.y), v.z);
}

float mean3(vec3 v) {
    return dot(v, vec3(1.0 / 3.0));
}
