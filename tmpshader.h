#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/effects/SkRuntimeEffect.h"

static const char* kMetaballsSkSL = R"(
uniform float2 iResolution;
uniform float  iTime;

half4 main(float2 fragCoord) {
    float2 uv = fragCoord / iResolution;
    float2 p = uv * 2.0 - 1.0;
    p.x *= iResolution.x / iResolution.y;

    float v = 0.0;

    float2 c1 = float2(0.35 * sin(iTime * 1.2), 0.30 * cos(iTime * 1.6));
    float2 c2 = float2(0.45 * cos(iTime * 0.9 + 1.0), 0.35 * sin(iTime * 1.3 + 2.0));
    float2 c3 = float2(0.40 * sin(iTime * 1.5 + 2.5), 0.25 * cos(iTime * 1.1 + 0.7));
    float2 c4 = float2(0.30 * cos(iTime * 1.8 + 4.0), 0.40 * sin(iTime * 0.8 + 1.7));

    v += 0.18 / length(p - c1);
    v += 0.20 / length(p - c2);
    v += 0.22 / length(p - c3);
    v += 0.18 / length(p - c4);

    float blobs = smoothstep(0.9, 1.0, v);
    float glow = smoothstep(0.55, 1.0, v) * 0.35;

    float3 bg = float3(0.03, 0.04, 0.08) + 0.08 * float3(uv.y, uv.x * 0.5, 1.0 - uv.y);

    float3 blobColor = float3(0.2, 0.8, 1.0);
    blobColor += 0.25 * sin(float3(0.0, 1.2, 2.4) + iTime * 2.0);

    float3 color = bg;
    color += glow * float3(0.1, 0.5, 0.9);
    color = mix(color, blobColor, blobs);

    return half4(color, 1.0);
}
)";

void DrawMetaballsBackground(SkCanvas* canvas, int width, int height, float timeSeconds) {
    auto result = SkRuntimeEffect::MakeForShader(SkString(kMetaballsSkSL));
    if (!result.effect) {
        return;
    }

    SkRuntimeShaderBuilder builder(result.effect);
    builder.uniform("iResolution") = SkV2{(float)width, (float)height};
    builder.uniform("iTime") = timeSeconds;

    SkPaint paint;
    paint.setShader(builder.makeShader());

    canvas->drawRect(SkRect::MakeWH((float)width, (float)height), paint);
}