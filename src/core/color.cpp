#include "color.h"
#include "machado_tables.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cvs {
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 mul(const Mat3 &m, Vec3 v) {
    return {dot({m.v[0], m.v[1], m.v[2]}, v), dot({m.v[3], m.v[4], m.v[5]}, v),
            dot({m.v[6], m.v[7], m.v[8]}, v)};
}
Mat3 mul(const Mat3 &a, const Mat3 &b) {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                r.at(i, j) += a.at(i, k) * b.at(k, j);
    return r;
}
Mat3 inverse(const Mat3 &m) {
    const auto &a = m.v;
    Mat3 r{{a[4] * a[8] - a[5] * a[7], a[2] * a[7] - a[1] * a[8], a[1] * a[5] - a[2] * a[4],
            a[5] * a[6] - a[3] * a[8], a[0] * a[8] - a[2] * a[6], a[2] * a[3] - a[0] * a[5],
            a[3] * a[7] - a[4] * a[6], a[1] * a[6] - a[0] * a[7], a[0] * a[4] - a[1] * a[3]}};
    const float d = a[0] * r.v[0] + a[1] * r.v[3] + a[2] * r.v[6];
    if (std::abs(d) < 1e-12f)
        throw std::invalid_argument("Singular matrix");
    for (auto &x : r.v)
        x /= d;
    return r;
}
static Mat3 mix(const Mat3 &a, const Mat3 &b, float t) {
    Mat3 r;
    for (int i = 0; i < 9; ++i)
        r.v[i] = a.v[i] + t * (b.v[i] - a.v[i]);
    return r;
}
float decode_srgb(float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
}
float encode_srgb(float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x <= 0.0031308f ? x * 12.92f : 1.055f * std::pow(x, 1.f / 2.4f) - 0.055f;
}
const std::array<PresetInfo, 15> &presets() {
    static const std::array<PresetInfo, 15> p{
        {{"protanopia", "Protanopia",
          "Machado 2009; 100% is dichromacy. Brettel is also available.", false, 1},
         {"protanomaly", "Protanomaly", "Machado 2009 anomalous trichromacy severity model.", false,
          .5f},
         {"deuteranopia", "Deuteranopia",
          "Machado 2009; 100% is dichromacy. Brettel is also available.", false, 1},
         {"deuteranomaly", "Deuteranomaly", "Machado 2009 anomalous trichromacy severity model.",
          false, .5f},
         {"tritanopia", "Tritanopia",
          "Brettel 1997 two-plane sRGB adaptation. Partial severity is a blend.", false, 1},
         {"tritanomaly", "Tritanomaly [approximation]",
          "Brettel blend; partial tritan loss lacks a validated anomaly model here.", true, .5f},
         {"achromatopsia", "Achromatopsia [approximation]",
          "Linear luminance grayscale; does not model acuity, rods or photophobia.", true, 1},
         {"achromatomaly", "Achromatomaly [approximation]",
          "Partial luminance grayscale; not a clinical prediction.", true, .6f},
         {"blue-cone-monochromacy", "Blue-cone monochromacy [experimental]",
          "S-cone weighted grayscale proxy; omits rod contribution and adaptation.", true, 1},
         {"cone-monochromacy", "Cone monochromacy [experimental]",
          "Selected surviving cone weighted grayscale; spatial effects omitted.", true, 1},
         {"acquired-red-green", "Acquired red-green [experimental]",
          "Deutan model proxy; acquired causes and field changes are not simulated.", true, .6f},
         {"acquired-blue-yellow", "Acquired blue-yellow [experimental]",
          "Tritan projection proxy; acquired disease effects are not simulated.", true, .6f},
         {"desaturation", "Generalized desaturation [artistic]",
          "Linear-light saturation reduction; not a physiological model.", true, .5f},
         {"custom-lms", "Custom L/M/S response [experimental]",
          "Smith-Pokorny LMS gain transform; cone gain is not cone loss physiology.", true, 1},
         {"custom-matrix", "Custom RGB matrix [artistic]",
          "User-defined 3x3 transform in linear sRGB.", true, 1}}};
    return p;
}
Preset preset_from_id(std::string_view id) {
    for (int i = 0; i < 15; ++i)
        if (id == presets()[i].id)
            return static_cast<Preset>(i);
    throw std::invalid_argument("Unknown preset id");
}
static void range(float n, float lo, float hi, const char *name) {
    if (!std::isfinite(n) || n < lo || n > hi)
        throw std::invalid_argument(name);
}
void validate(const Settings &s) {
    if (static_cast<int>(s.preset) < 0 || s.preset >= Preset::Count)
        throw std::invalid_argument("Invalid preset");
    if (static_cast<int>(s.model) < 0 || static_cast<int>(s.model) > 2)
        throw std::invalid_argument("Invalid model");
    range(s.severity, 0, 1, "Severity must be 0..1");
    range(s.saturation, 0, 2, "Saturation must be 0..2");
    range(s.brightness, -2, 2, "Brightness must be -2..2 EV");
    range(s.contrast, 0, 2, "Contrast must be 0..2");
    for (float x : s.cones)
        range(x, 0, 2, "Cone gains must be 0..2");
    for (float x : s.custom.v)
        range(x, -8, 8, "Matrix coefficients must be -8..8");
    if (s.survivingCone < 0 || s.survivingCone > 2)
        throw std::invalid_argument("Invalid surviving cone");
}
Mat3 machado_matrix(int d, float severity) {
    if (d < 0 || d > 2)
        throw std::invalid_argument("Invalid deficiency");
    range(severity, 0, 1, "Invalid severity");
    float p = severity * 10;
    int lo = static_cast<int>(p);
    if (lo >= 10)
        return machado[d][10];
    return mix(machado[d][lo], machado[d][lo + 1], p - static_cast<float>(lo));
}
// Smith-Pokorny 1975 / Judd-Vos sRGB adaptation and spectral anchors,
// as documented by DaltonLens (MIT). White-neutral adaptation, not calibrated spectra.
static constexpr Mat3 lmsFromXyz{
    {.15514f, .54312f, -.03286f, -.15514f, .45684f, .03286f, 0, 0, .01608f}};
static constexpr Mat3 xyzFromRgb{
    {.409568f, .355041f, .179167f, .213389f, .706743f, .079868f, .0186297f, .114620f, .912367f}};
static Mat3 lmsFromRgb() {
    return mul(lmsFromXyz, xyzFromRgb);
}
Brettel brettel_matrices(int d) {
    if (d < 0 || d > 2)
        throw std::invalid_argument("Invalid deficiency");
    const auto lms = lmsFromRgb();
    auto white = mul(lms, Vec3{1, 1, 1});
    Vec3 axis{};
    axis[d] = 1;
    Vec3 a =
        mul(lmsFromXyz, d == 2 ? Vec3{.05699f, .16987f, .5864f} : Vec3{.13287f, .11284f, .9422f});
    Vec3 b =
        mul(lmsFromXyz, d == 2 ? Vec3{.16161f, .061f, .00001f} : Vec3{.84394f, .91558f, .00197f});
    Vec3 n = cross(white, axis);
    if (dot(n, a) < 0)
        std::swap(a, b);
    auto projection = [&](Vec3 anchor) {
        auto normal = cross(white, anchor);
        Mat3 p = identity;
        for (int j = 0; j < 3; ++j)
            p.at(d, j) = j == d ? 0 : -normal[j] / normal[d];
        return mul(mul(inverse(lms), p), lms);
    };
    Vec3 sep{};
    for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 3; ++k)
            sep[j] += n[k] * lms.at(k, j);
    // Positive rescaling improves floating point precision without changing plane selection.
    float scale = std::max({std::abs(sep.x), std::abs(sep.y), std::abs(sep.z)});
    sep = {sep.x / scale, sep.y / scale, sep.z / scale};
    return {projection(a), projection(b), sep};
}
static Mat3 grayscale(Vec3 weights) {
    return {{weights.x, weights.y, weights.z, weights.x, weights.y, weights.z, weights.x, weights.y,
             weights.z}};
}
Transform make_transform(const Settings &s) {
    validate(s);
    Mat3 a = identity, b = identity;
    Vec3 sep{};
    int d = -1;
    switch (s.preset) {
    case Preset::Protanopia:
    case Preset::Protanomaly:
        d = 0;
        break;
    case Preset::Deuteranopia:
    case Preset::Deuteranomaly:
    case Preset::AcquiredRedGreen:
        d = 1;
        break;
    case Preset::Tritanopia:
    case Preset::Tritanomaly:
    case Preset::AcquiredBlueYellow:
        d = 2;
        break;
    default:
        break;
    }
    if (d >= 0) {
        bool useBrettel = s.model == Model::Brettel || (s.model == Model::Recommended && d == 2);
        if (useBrettel) {
            auto m = brettel_matrices(d);
            a = mix(identity, m.first, s.severity);
            b = mix(identity, m.second, s.severity);
            sep = m.separator;
        } else
            a = b = machado_matrix(d, s.severity);
    } else {
        Mat3 m = grayscale({.2126f, .7152f, .0722f});
        if (s.preset == Preset::BlueConeMonochromacy || s.preset == Preset::ConeMonochromacy) {
            int cone = s.preset == Preset::BlueConeMonochromacy ? 2 : s.survivingCone;
            auto lms = lmsFromRgb();
            float sum = lms.at(cone, 0) + lms.at(cone, 1) + lms.at(cone, 2);
            m = grayscale({lms.at(cone, 0) / sum, lms.at(cone, 1) / sum, lms.at(cone, 2) / sum});
        } else if (s.preset == Preset::CustomLMS) {
            Mat3 gain{};
            for (int i = 0; i < 3; ++i)
                gain.at(i, i) = s.cones[i];
            auto lms = lmsFromRgb();
            m = mul(mul(inverse(lms), gain), lms);
        } else if (s.preset == Preset::CustomMatrix)
            m = s.custom;
        a = b = mix(identity, m, s.severity);
    }
    Mat3 post = mix(grayscale({.2126f, .7152f, .0722f}), identity, s.saturation);
    Transform t{};
    for (int i = 0; i < 3; ++i) {
        t.first[i] = {a.at(i, 0), a.at(i, 1), a.at(i, 2), 0};
        t.second[i] = {b.at(i, 0), b.at(i, 1), b.at(i, 2), 0};
        t.artistic[i] = {post.at(i, 0), post.at(i, 1), post.at(i, 2), 0};
    }
    t.separator = {sep.x, sep.y, sep.z, 0};
    t.adjustment = {std::exp2(s.brightness), s.contrast, .18f, 0};
    return t;
}
Vec3 transform_pixel(Vec3 c, const Transform &t) {
    c = {decode_srgb(c.x), decode_srgb(c.y), decode_srgb(c.z)};
    auto apply = [](const Float4 *r, Vec3 x) {
        return Vec3{dot({r[0].x, r[0].y, r[0].z}, x), dot({r[1].x, r[1].y, r[1].z}, x),
                    dot({r[2].x, r[2].y, r[2].z}, x)};
    };
    const auto *m = dot(c, {t.separator.x, t.separator.y, t.separator.z}) < 0 ? t.second : t.first;
    c = apply(t.artistic, apply(m, c));
    for (int i = 0; i < 3; ++i)
        c[i] =
            encode_srgb((c[i] * t.adjustment.x - t.adjustment.z) * t.adjustment.y + t.adjustment.z);
    return c;
}
} // namespace cvs
