#pragma once
#include <array>
#include <string_view>

namespace cvs {
struct Vec3 {
    float x{}, y{}, z{};
    float &operator[](int i) {
        return i == 0 ? x : i == 1 ? y : z;
    }
    float operator[](int i) const {
        return i == 0 ? x : i == 1 ? y : z;
    }
};
struct Mat3 {
    std::array<float, 9> v{};
    float &at(int r, int c) {
        return v[r * 3 + c];
    }
    float at(int r, int c) const {
        return v[r * 3 + c];
    }
};
constexpr Mat3 identity{{1, 0, 0, 0, 1, 0, 0, 0, 1}};
Vec3 mul(const Mat3 &, Vec3);
Mat3 mul(const Mat3 &, const Mat3 &);
Mat3 inverse(const Mat3 &);
float dot(Vec3, Vec3);
float decode_srgb(float);
float encode_srgb(float);

enum class Preset : int {
    Protanopia,
    Protanomaly,
    Deuteranopia,
    Deuteranomaly,
    Tritanopia,
    Tritanomaly,
    Achromatopsia,
    Achromatomaly,
    BlueConeMonochromacy,
    ConeMonochromacy,
    AcquiredRedGreen,
    AcquiredBlueYellow,
    Desaturation,
    CustomLMS,
    CustomMatrix,
    Count
};
enum class Model : int { Recommended, Machado, Brettel };
struct PresetInfo {
    const char *id;
    const char *name;
    const char *description;
    bool experimental;
    float severity;
};
const std::array<PresetInfo, static_cast<int>(Preset::Count)> &presets();
Preset preset_from_id(std::string_view);
struct Settings {
    Preset preset{Preset::Deuteranopia};
    Model model{Model::Recommended};
    float severity{1};
    std::array<float, 3> cones{1, 1, 1};
    int survivingCone{0};
    float saturation{1}, brightness{0}, contrast{1};
    Mat3 custom{identity};
};
void validate(const Settings &);

// Mirrors the embedded HLSL constant buffer. Matrices are explicit rows.
struct alignas(16) Float4 {
    float x{}, y{}, z{}, w{};
};
struct alignas(16) Transform {
    Float4 first[3], second[3], separator, artistic[3], adjustment;
};
static_assert(sizeof(Transform) == 176);
Transform make_transform(const Settings &);
Vec3 transform_pixel(Vec3 srgb, const Transform &);
Mat3 machado_matrix(int deficiency, float severity);
struct Brettel {
    Mat3 first, second;
    Vec3 separator;
};
Brettel brettel_matrices(int deficiency);
} // namespace cvs
