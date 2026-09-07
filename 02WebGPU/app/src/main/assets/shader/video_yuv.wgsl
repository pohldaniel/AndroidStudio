const position = array(
    vec2(-1.0, -1.0), vec2(3.0f,  -1.0), vec2(-1.0, 3.0)
);

struct VertexOutput {
  @builtin(position) position : vec4f, 
  @location(0) uv : vec2f,
};

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
  var output : VertexOutput;
  output.position = vec4f(position[vertexIndex], 0.0, 1.0);
  output.uv       = position[vertexIndex] * vec2f(0.5, -0.5) + vec2f(0.5);
  return output;
}

@group(0) @binding(0) var videoSampler: sampler;
@group(0) @binding(1) var videoTextureY: texture_2d<f32>;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    
    var video_u = in.uv.x;
    var video_v = in.uv.y;
    //video_v = 1.0 - video_v;

    // --- 2. SCHNELLER LINEARER ATLAS-ZUGRIFF ---
    
    // Y-Bereich (Helligkeit): Volle Breite, obere 66.6%
    let y_uv = vec2<f32>(video_u, video_v * (2.0 / 3.0));
    let y = textureSample(videoTextureY, videoSampler, y_uv).r;

    // U-Bereich (Chroma Cb): Mittlerer Block (66.6% - 83.3%), linke Hälfte (0.0 - 0.5)
    let u_uv = vec2<f32>(video_u * 0.5, (2.0 / 3.0) + (video_v * (1.0 / 6.0)));
    let u = textureSample(videoTextureY, videoSampler, u_uv).r;

    // V-Bereich (Chroma Cr): Unterster Block (83.3% - 100%), linke Hälfte (0.0 - 0.5)
    let v_uv = vec2<f32>(video_u * 0.5, (5.0 / 6.0) + (video_v * (1.0 / 6.0)));
    let v = textureSample(videoTextureY, videoSampler, v_uv).r;

    // --- 3. SPEZIFISCHE SMPTE170M (BT.601) LIMITED-RANGE MATRIX ---
    let y_norm = y - 0.062745;
    let u_norm = u - 0.501961;
    let v_norm = v - 0.501961;

    let r = 1.164384 * y_norm + 1.596027 * v_norm;
    let g = 1.164384 * y_norm - 0.391762 * u_norm - 0.812968 * v_norm;
    let b = 1.164384 * y_norm + 2.017232 * u_norm;

    return vec4<f32>(r,g,b, 1.0);
}