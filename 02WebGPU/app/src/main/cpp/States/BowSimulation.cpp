#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION

#include <nuklear/nuklear.h>

#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>
#include <States/VolumeRendering.h>
#include <States/Collada.h>

#include "AssetIO.h"
#include "BowSimulation.h"

#define MAX_TOUCH_POINTERS 5
extern float c_touch_x[MAX_TOUCH_POINTERS];
extern float c_touch_y[MAX_TOUCH_POINTERS];
extern bool c_touch_active[MAX_TOUCH_POINTERS];

struct nk_context ctx;
struct nk_buffer commands;
struct nk_convert_config config;
static uint8_t cpu_vertex_linear_buffer[MAX_VERTEX_MEMORY];
static uint8_t cpu_index_linear_buffer[MAX_INDEX_MEMORY];
const struct nk_draw_command* cmd = NULL;
struct nk_buffer vbuf, ibuf;
struct nk_font_atlas atlas;
struct nk_font* default_font;
struct nk_font* custom_font;
struct nk_image playIcon;
struct nk_vec2 current_pos;

const float BASE_ROW_DYN = 30.0f;
const float BASE_ROW_STAT = 32.0f;
const float BASE_FONT_SIZE = 16.0f;

static BowSimulation::Reticle my_reticle;
static bool is_reticle_initialized = false;
static bool is_bow_drawn = false;

BowSimulation::BowSimulation(StateMachine& machine) : State(machine, States::BOW_SIMULATION) {

    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
	m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f,  -1.0f, 1.0f);
	m_camera.lookAt(4.0f, 0.1f * 180.0f, 0.0f, 0.1f * 180.0f);
	m_camera.setMovingSpeed(5.0f);
	m_camera.setRotationSpeed(0.1f);

	m_vertexBuffer.createBuffer(MAX_VERTEX_MEMORY, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
	m_indexBuffer.createBuffer(MAX_INDEX_MEMORY, WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
	m_texture.createEmpty(1u, 1u, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst, WGPUTextureFormat_RGBA8Unorm);
	m_textureIcon.loadFromFile("textures/ui-icons-buttons-set-blue.png", true);

	uint8_t white_pixel[4] = { 255, 255, 255, 255 };

	WGPUTexelCopyTextureInfo destination = {};
	destination.texture = m_texture.getTexture();
	destination.mipLevel = 0u;
	destination.origin = { 0u, 0u, 0u };
	destination.aspect = WGPUTextureAspect_All;

	WGPUTexelCopyBufferLayout source = {};
	source.offset = 0u;
	source.bytesPerRow = 4u;
	source.rowsPerImage = 1u;

	WGPUExtent3D size = { 1u , 1u , 1u };
	wgpuQueueWriteTexture(wgpContext.queue, &destination, white_pixel, 4u, &source, &size);

	memset(&config, 0, sizeof(config));
	config.shape_AA = NK_ANTI_ALIASING_ON;
	config.line_AA = NK_ANTI_ALIASING_ON;
	config.circle_segment_count = 22;
	config.curve_segment_count = 22;
	config.arc_segment_count = 22;
	config.global_alpha = 1.0f;
	config.null.texture.ptr = m_texture.getTextureView();
	config.null.uv = nk_vec2(0.5f, 0.5f);

	static const struct nk_draw_vertex_layout_element vertex_layout[] = {
			{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, 0},
			{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, 2 * sizeof(float)},
			{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, 4 * sizeof(float)},
			{NK_VERTEX_LAYOUT_END}
	};

	config.vertex_layout = vertex_layout;
	config.vertex_size = 20;
	config.vertex_alignment = 4;

	nk_init_default(&ctx, NULL);
	nk_buffer_init_default(&commands);
	nk_buffer_init_fixed(&vbuf, cpu_vertex_linear_buffer, MAX_VERTEX_MEMORY);
	nk_buffer_init_fixed(&ibuf, cpu_index_linear_buffer, MAX_INDEX_MEMORY);

	nk_font_atlas_init_default(&atlas);
	nk_font_atlas_begin(&atlas);

	//default_font = nk_font_atlas_add_default(&atlas, BASE_FONT_SIZE, NULL);

	uint8_t* data; uint32_t _size;
	AssetIO::LoadAsset("fonts/upheavtt.ttf", data, _size);

	struct nk_font_config config_font = nk_font_config(0.0f);
	config_font.oversample_h = 3;
	config_font.oversample_v = 3;
	custom_font = nk_font_atlas_add_from_memory(&atlas, data, _size, BASE_FONT_SIZE, &config_font);
	AssetIO::Free(data);

	const void* image_pixels;
	int atlas_width, atlas_height;
	image_pixels = nk_font_atlas_bake(&atlas, &atlas_width, &atlas_height, NK_FONT_ATLAS_RGBA32);

	m_textureFont.createEmpty(atlas_width, atlas_height, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst, WGPUTextureFormat_RGBA8Unorm);

	WGPUTexelCopyTextureInfo font_destination = {};
	font_destination.texture = m_textureFont.getTexture();
	font_destination.mipLevel = 0u;
	font_destination.origin = { 0u, 0u, 0u };
	font_destination.aspect = WGPUTextureAspect_All;

	WGPUTexelCopyBufferLayout font_source = {};
	font_source.offset = 0u;
	font_source.bytesPerRow = (uint32_t)atlas_width * 4;
	font_source.rowsPerImage = (uint32_t)atlas_height;

	WGPUExtent3D font_size = { (uint32_t)atlas_width, (uint32_t)atlas_height, 1u };
	wgpuQueueWriteTexture(wgpContext.queue, &font_destination, image_pixels, (size_t)(atlas_width * atlas_height * 4), &font_source, &font_size);

	nk_handle font_handle;
	font_handle.ptr = m_textureFont.getTextureView();

	nk_font_atlas_end(&atlas, font_handle, &config.null);
	nk_style_set_font(&ctx, &custom_font->handle);
	nk_style_default(&ctx);

	playIcon = nk_subimage_ptr(m_textureIcon.getTextureView(), 960, 560, nk_rect(30.0f, 25.0f, 120.0f, 122.0f));

	wgpContext.addSahderModule("NUKLEAR", "shader/nuklear.wgsl");
	wgpContext.createRenderPipeline("NUKLEAR", "RP_NUKLEAR", VL_GUI, std::bind(&BowSimulation::OnBindGroupLayouts, this), 1u,
	                                WGPUPrimitiveTopology_TriangleList,
	                                WGPUTextureFormat_Undefined,
	                                WGPUTextureFormat_Undefined,
	                                WGPUCompareFunction_Always,
	                                { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None });

	m_trackball.reshape(wgpWidth, wgpHeight);

	m_uniformBuffer.createBuffer(sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
	wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &m_camera.getOrthographicMatrix(), sizeof(glm::mat4));

	wgpContext.setClearColor({ 0.2f, 0.0f, 0.2f, 1.0f });

	wgpContext.OnDraw = std::bind(&BowSimulation::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
	m_bindgroup = createBindGroup();
	m_bindgroupFont = createBindGroupFont();
	m_bindgroupIcon = createBindGroupIcon();

	if (!is_reticle_initialized) {
		init_reticle(my_reticle, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
		is_reticle_initialized = true;
	}

}

BowSimulation::~BowSimulation() {
   
}

void BowSimulation::fixedUpdate() {

}

void BowSimulation::update() {
	nk_input_begin(&ctx);
	nk_input_motion(&ctx, (int)c_touch_x[0], (int)c_touch_y[0]);
	nk_input_button(&ctx, NK_BUTTON_LEFT, (int)c_touch_x[0], (int)c_touch_y[0], 1);

	if (m_wasHovered1)
		nk_input_scroll(&ctx, nk_vec2(0.0f, m_scrollDelta));

	nk_input_end(&ctx);

	m_scrollDelta = 0.0f;
	m_isHovered = nk_window_is_any_hovered(&ctx);

    m_trackball.idle();
}

void BowSimulation::render() {
    wgpDraw();
}

void BowSimulation::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
	wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &m_camera.getOrthographicMatrix(), sizeof(glm::mat4));

	custom_font->handle.height = BASE_FONT_SIZE * m_uiScale;
	custom_font->scale = m_uiScale;
	int joystick_finger = -1;
	int action_finger = -1;
	for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
		if (c_touch_active[i]) {
			// Liegt dieser Finger auf der linken oder rechten Bildschirmhälfte?
			if (c_touch_x[i] < (static_cast<float>(wgpWidth) / 2.0f)) {
				if (joystick_finger == -1) joystick_finger = i; // Erster Finger links ist der Joystick
			} else {
				if (action_finger == -1) action_finger = i;   // Erster Finger rechts ist der Action-Button
			}
		}
	}

	float joy_x = 0.0f, joy_y = 0.0f;

	struct nk_color old_background = ctx.style.window.background;
	ctx.style.window.background = nk_rgba(0, 0, 0, 0);

	struct nk_color old_border = ctx.style.window.border_color;
	ctx.style.window.border_color = nk_rgba(0, 0, 0, 0);

	nk_style_push_style_item(&ctx, &ctx.style.window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));
	nk_style_push_color(&ctx, &ctx.style.window.background, nk_rgba(0, 0, 0, 0));
	nk_style_push_color(&ctx, &ctx.style.window.border_color, nk_rgba(0, 0, 0, 0));

	nk_style_push_vec2(&ctx, &ctx.style.window.padding, nk_vec2(0, 0));
	nk_style_push_vec2(&ctx, &ctx.style.window.group_padding, nk_vec2(0, 0));
	nk_style_push_vec2(&ctx, &ctx.style.window.spacing, nk_vec2(0, 0));
	nk_style_push_float(&ctx, &ctx.style.window.border, 0.0f);

	if (nk_begin(&ctx, "HUD_Controls", nk_rect(20, static_cast<float>(wgpHeight) - 400 * m_uiScale, 180 * m_uiScale, 180 * m_uiScale),
	             NK_WINDOW_NO_SCROLLBAR)){

		float joystick_size = 180.0f * m_uiScale;
		nk_layout_row_static(&ctx, joystick_size, joystick_size, 1);

		JoystickResult input_vector = nk_virtual_joystick(&ctx, joystick_size, joystick_finger);

		if (input_vector.is_active) {
				joy_x = input_vector.x;
				joy_y = input_vector.y;
		}
	}
	nk_end(&ctx);

	nk_style_pop_float(&ctx);
	nk_style_pop_vec2(&ctx);
	nk_style_pop_vec2(&ctx);
	nk_style_pop_vec2(&ctx);

	nk_style_pop_color(&ctx);
	nk_style_pop_color(&ctx);
	nk_style_pop_style_item(&ctx);

	ctx.style.window.background = old_background;
	ctx.style.window.border_color = old_border;

	nk_style_push_vec2(&ctx, &ctx.style.window.padding, nk_vec2(0, 0));
	nk_style_push_vec2(&ctx, &ctx.style.window.group_padding, nk_vec2(0, 0));
	nk_style_push_vec2(&ctx, &ctx.style.window.spacing, nk_vec2(0, 0));
	nk_style_push_float(&ctx, &ctx.style.window.border, 0.0f);

	nk_style_push_style_item(&ctx, &ctx.style.window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));
	nk_style_push_color(&ctx, &ctx.style.window.background, nk_rgba(0, 0, 0, 0));
	nk_style_push_color(&ctx, &ctx.style.window.border_color, nk_rgba(0, 0, 0, 0));

	float btn_box_size = 140.0f * m_uiScale;
	float margin = 40.0f * m_uiScale;

	float btn_x = static_cast<float>(wgpWidth) - btn_box_size - margin;
	float btn_y = static_cast<float>(wgpHeight) - 400 * m_uiScale;

	if (nk_begin(&ctx, "HUD_ActionButton", nk_rect(btn_x, btn_y, btn_box_size, btn_box_size), NK_WINDOW_NO_SCROLLBAR)) {

		nk_layout_row_static(&ctx, btn_box_size, btn_box_size, 1);

		// Button aufrufen
		nk_circular_action_button(&ctx, "Spannen", btn_box_size, action_finger);
			if (action_finger != -1 && c_touch_active[action_finger]) {
				// Finger ist drauf -> Bogen wird aktiv gespannt
				is_bow_drawn = true;
			} else {
				// Finger ist NICHT drauf oder wurde gerade angehoben
				if (is_bow_drawn) {
					// Pfeilschuss abfeuern!
					//my_game_engine.fire_arrow(my_reticle.x, my_reticle.y);

					// Sofort auf false setzen, damit update_reticle im GLEICHEN Frame
					// weiß, dass der Bogen entspannt ist!
					is_bow_drawn = false;
				}
			}

	}
	nk_end(&ctx);


	nk_style_pop_color(&ctx);
	nk_style_pop_color(&ctx);
	nk_style_pop_style_item(&ctx);
	nk_style_pop_float(&ctx);
	nk_style_pop_vec2(&ctx);
	nk_style_pop_vec2(&ctx);
	nk_style_pop_vec2(&ctx);

	update_reticle(my_reticle, joy_x, joy_y, is_bow_drawn, m_dt, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
	draw_reticle(&ctx, my_reticle, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));

	nk_buffer_clear(&vbuf);
	nk_buffer_clear(&ibuf);

	nk_convert(&ctx, &commands, &vbuf, &ibuf, &config);
	m_initUi = false;

	uint32_t vertex_count = vbuf.needed / sizeof(nk_webgpu_vertex);
	nk_webgpu_vertex* vertices = (nk_webgpu_vertex*)cpu_vertex_linear_buffer;
	ibuf.needed = (ibuf.needed + 3) & ~3;
	wgpuQueueWriteBuffer(wgpContext.queue, m_vertexBuffer.getBuffer(), 0, cpu_vertex_linear_buffer, vbuf.needed);
	wgpuQueueWriteBuffer(wgpContext.queue, m_indexBuffer.getBuffer(), 0, cpu_index_linear_buffer, ibuf.needed);

	WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
	wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_NUKLEAR"));

	wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_vertexBuffer.getBuffer(), 0, MAX_VERTEX_MEMORY);
	wgpuRenderPassEncoderSetIndexBuffer(renderPassEncoder, m_indexBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0, MAX_INDEX_MEMORY);

	uint32_t index_offset = 0;
	nk_draw_foreach(cmd, &ctx, &commands) {
		if (!cmd->elem_count) continue;
		WGPUTextureView active_view = (WGPUTextureView)cmd->texture.ptr;
		if (active_view == m_textureFont.getTextureView()) {
			wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindgroupFont, 0u, NULL);
		}else if (active_view == m_textureIcon.getTextureView()) {
			wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindgroupIcon, 0u, NULL);
		}else {
			wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindgroup, 0u, NULL);
		}

		float scissor_x = cmd->clip_rect.x < 0.0f ? 0.0f : cmd->clip_rect.x;
		float scissor_y = cmd->clip_rect.y < 0.0f ? 0.0f : cmd->clip_rect.y;
		float scissor_w = cmd->clip_rect.w;
		float scissor_h = cmd->clip_rect.h;

		if (scissor_x + scissor_w > static_cast<float>(wgpWidth)) {
			scissor_w = static_cast<float>(wgpWidth) - scissor_x;
		}

		if (scissor_y + scissor_h > static_cast<float>(wgpHeight)) {
			scissor_h = static_cast<float>(wgpHeight) - scissor_y;
		}

		if (scissor_w <= 0.0f || scissor_h <= 0.0f) {
			continue;
		}

		wgpuRenderPassEncoderSetScissorRect(renderPassEncoder, (uint32_t)scissor_x, (uint32_t)scissor_y, (uint32_t)scissor_w, (uint32_t)scissor_h);
		wgpuRenderPassEncoderDrawIndexed(renderPassEncoder, cmd->elem_count, 1, index_offset, 0, 0);
		index_offset += cmd->elem_count;
	}
	wgpuRenderPassEncoderEnd(renderPassEncoder);
	wgpuRenderPassEncoderRelease(renderPassEncoder);
	nk_clear(&ctx);
	nk_buffer_clear(&commands);
}

void BowSimulation::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(72.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, -1.0f, 1.0f);
    m_trackball.reshape(wgpWidth, wgpHeight);
}

void BowSimulation::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new VolumeRendering(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> BowSimulation::OnBindGroupLayouts() {
	std::vector<WGPUBindGroupLayout> bindingLayouts(1);

	std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

	bindingLayoutEntries[0].binding = 0u;
	bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
	bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
	bindingLayoutEntries[0].buffer.minBindingSize = sizeof(glm::mat4);

	bindingLayoutEntries[1].binding = 1u;
	bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
	bindingLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

	bindingLayoutEntries[2].binding = 2u;
	bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
	bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType_Float;
	bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
	bindingLayoutEntries[2].texture.multisampled = WGPU_FALSE;

	WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
	bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
	bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

	bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);
	return bindingLayouts;
}

WGPUBindGroup BowSimulation::createBindGroup() {
	std::vector<WGPUBindGroupEntry> bindGroupEntries(3);
	bindGroupEntries[0].binding = 0u;
	bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
	bindGroupEntries[0].offset = 0u;
	bindGroupEntries[0].size = sizeof(glm::mat4);

	bindGroupEntries[1].binding = 1u;
	bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

	bindGroupEntries[2].binding = 2u;
	bindGroupEntries[2].textureView = m_texture.getTextureView();


	WGPUBindGroupDescriptor bindGroupDesc = {};
	bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_NUKLEAR"), 0u);
	bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
	bindGroupDesc.entries = bindGroupEntries.data();

	return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup BowSimulation::createBindGroupFont() {
	std::vector<WGPUBindGroupEntry> bindGroupEntries(3);
	bindGroupEntries[0].binding = 0u;
	bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
	bindGroupEntries[0].offset = 0u;
	bindGroupEntries[0].size = sizeof(glm::mat4);

	bindGroupEntries[1].binding = 1u;
	bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

	bindGroupEntries[2].binding = 2u;
	bindGroupEntries[2].textureView = m_textureFont.getTextureView();


	WGPUBindGroupDescriptor bindGroupDesc = {};
	bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_NUKLEAR"), 0u);
	bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
	bindGroupDesc.entries = bindGroupEntries.data();

	return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup BowSimulation::createBindGroupIcon() {
	std::vector<WGPUBindGroupEntry> bindGroupEntries(3);
	bindGroupEntries[0].binding = 0u;
	bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
	bindGroupEntries[0].offset = 0u;
	bindGroupEntries[0].size = sizeof(glm::mat4);

	bindGroupEntries[1].binding = 1u;
	bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

	bindGroupEntries[2].binding = 2u;
	bindGroupEntries[2].textureView = m_textureIcon.getTextureView();


	WGPUBindGroupDescriptor bindGroupDesc = {};
	bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_NUKLEAR"), 0u);
	bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
	bindGroupDesc.entries = bindGroupEntries.data();

	return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

BowSimulation::JoystickResult BowSimulation::nk_virtual_joystick(struct nk_context* ctx, float size_px, int touch_id) {
	JoystickResult result;

	// 1. Platz im Layout reservieren (MUSS immer laufen!)
	struct nk_rect bounds;
	nk_widget(&bounds, ctx);

	float radius_base = bounds.w / 2.0f;
	float radius_stick = radius_base * 0.4f;
	struct nk_vec2 center = nk_vec2(bounds.x + radius_base, bounds.y + radius_base);

	static bool joystick_captured = false;
	struct nk_vec2 stick_pos = center;

	// --- LOGIK-BLOCK (Nur ausführen bei gültiger ID) ---
	if (touch_id >= 0 && touch_id < MAX_TOUCH_POINTERS) {
		extern float c_touch_x[MAX_TOUCH_POINTERS];
		extern float c_touch_y[MAX_TOUCH_POINTERS];
		extern bool c_touch_active[MAX_TOUCH_POINTERS];

		if (c_touch_active[touch_id]) {
			float tx = c_touch_x[touch_id];
			float ty = c_touch_y[touch_id];

			if (!joystick_captured) {
				if (tx >= bounds.x && tx <= (bounds.x + bounds.w) &&
				    ty >= bounds.y && ty <= (bounds.y + bounds.h)) {
					joystick_captured = true;
				}
			}

			if (joystick_captured) {
				result.is_active = true;
				float dx = tx - center.x;
				float dy = ty - center.y;
				float distance = std::sqrt(dx * dx + dy * dy);
				float max_distance = radius_base - radius_stick;

				if (distance > 0.0f) {
					float nx = dx / distance;
					float ny = dy / distance;
					float clamped_dist = (distance > max_distance) ? max_distance : distance;

					stick_pos.x = center.x + nx * clamped_dist;
					stick_pos.y = center.y + ny * clamped_dist;

					result.x = (nx * clamped_dist) / max_distance;
					result.y = -((ny * clamped_dist) / max_distance);
				}
			}
		} else {
			joystick_captured = false;
		}
	} else {
		// Wenn touch_id == -1, erzwingen wir, dass das Capture gelöst wird
		joystick_captured = false;
	}

	// --- ZEICHEN-BLOCK (MUSS immer laufen, damit das HUD sichtbar bleibt!) ---
	struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
	if (canvas) {
		nk_fill_circle(canvas, nk_rect(bounds.x, bounds.y, bounds.w, bounds.h), nk_rgba(50, 50, 50, 150));
		nk_stroke_circle(canvas, nk_rect(bounds.x, bounds.y, bounds.w, bounds.h), 2.0f, nk_rgb(200, 200, 200));

		float sx = stick_pos.x - radius_stick;
		float sy = stick_pos.y - radius_stick;
		float sw = radius_stick * 2.0f;
		nk_fill_circle(canvas, nk_rect(sx, sy, sw, sw), nk_rgb(255, 100, 100));
	}

	return result;
}

bool BowSimulation::nk_circular_action_button(struct nk_context* ctx, const char* label, float size_px, int touch_id) {
	bool is_pressed = false;

	struct nk_rect bounds;
	nk_widget(&bounds, ctx);

	float radius = bounds.w / 2.0f;
	struct nk_vec2 center = nk_vec2(bounds.x + radius, bounds.y + radius);

	bool is_hovered = false;

	// --- LOGIK-BLOCK ---
	if (touch_id >= 0 && touch_id < MAX_TOUCH_POINTERS) {
		extern float c_touch_x[MAX_TOUCH_POINTERS];
		extern float c_touch_y[MAX_TOUCH_POINTERS];
		extern bool c_touch_active[MAX_TOUCH_POINTERS];

		if (c_touch_active[touch_id]) {
			float tx = c_touch_x[touch_id];
			float ty = c_touch_y[touch_id];
			float dx = tx - center.x;
			float dy = ty - center.y;
			if ((dx * dx + dy * dy) <= (radius * radius)) {
				is_hovered = true;
			}
		}

		static bool button_was_down = false;
		if (is_hovered && c_touch_active[touch_id]) {
			if (!button_was_down) {
				is_pressed = true;
				button_was_down = true;
			}
		} else if (!c_touch_active[touch_id]) {
			button_was_down = false;
		}
	}

	// --- ZEICHEN-BLOCK ---
	struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
	if (canvas) {
		struct nk_color btn_color = is_hovered ? nk_rgb(180, 50, 50) : nk_rgb(255, 80, 80);
		struct nk_color border_color = is_hovered ? nk_rgb(255, 255, 255) : nk_rgb(200, 200, 200);

		nk_fill_circle(canvas, bounds, btn_color);
		nk_stroke_circle(canvas, bounds, 3.0f, border_color);

		const struct nk_user_font* font = ctx->style.font;
		float text_width = font->width(font->userdata, font->height, label, nk_strlen(label));

		struct nk_vec2 text_pos;
		text_pos.x = center.x - (text_width / 2.0f);
		text_pos.y = center.y - (font->height / 2.0f);

		nk_draw_text(canvas, nk_rect(text_pos.x, text_pos.y, text_width, font->height),
		             label, nk_strlen(label), font, nk_rgb(0, 0, 0), nk_rgb(255, 255, 255));
	}

	return is_pressed;
}

void BowSimulation::init_reticle(Reticle& r, float screen_w, float screen_h) {
	r.x = screen_w / 2.0f;
	r.y = screen_h / 2.0f;
}

// Update-Logik für das Fadenkreuz
void BowSimulation::update_reticle(Reticle& r, float joystick_x, float joystick_y, bool is_drawing_bow, float dt, float screen_w, float screen_h) {
	// 1. Joystick-Eingabe (unverändert)
	r.x += joystick_x * r.speed * dt;
	r.y += (-joystick_y) * r.speed * dt;

	// 2. Muskelermüdung & Zittern berechnen
	float current_sway = r.sway_intensity;

	if (is_drawing_bow) {
		// Zeit läuft vorwärts, wenn der Bogen gespannt ist
		r.sway_time += dt * 5.0f;

		// Ermüdungs-Limit setzen: Maximal das 4-fache Zittern, sonst wird es unspielbar
		float fatigue_factor = 1.0f + (r.sway_time * 0.15f);
		if (fatigue_factor > 4.0f) fatigue_factor = 4.0f;

		current_sway = r.sway_intensity * fatigue_factor;
	} else {
		// --- HIER IST DIE KORREKTUR ---
		// Wenn der Bogen NICHT gespannt ist, beruhigt sich der Schütze sofort!
		r.sway_time = 0.0f;
		current_sway = r.sway_intensity * 0.2f; // Minimale Eigenbewegung im Ruhezustand
	}

	// Organische Lissajous-Kurve für das Zittern
	float sway_x = std::sin(r.sway_time * 1.5f) * std::cos(r.sway_time * 0.8f) * current_sway;
	float sway_y = std::cos(r.sway_time * 1.2f) * std::sin(r.sway_time * 0.9f) * current_sway;

	// 3. Fokus-Kreis Animation (Ausweiten / Zusammenziehen)
	if (is_drawing_bow) {
		if (r.focus_radius > r.min_focus) {
			r.focus_radius -= 80.0f * dt; // Etwas schnelleres Fokussieren
		}
		if (r.focus_radius < r.min_focus) r.focus_radius = r.min_focus;
	} else {
		// Wenn entspannt, öffnet sich der Ring sofort wieder auf die maximale Größe
		if (r.focus_radius < r.max_focus) {
			r.focus_radius += 150.0f * dt;
		}
		if (r.focus_radius > r.max_focus) r.focus_radius = r.max_focus;
	}

	// 4. Grenzen einhalten (Screen Clamping)
	if (r.x < 0) r.x = 0;
	if (r.x > screen_w) r.x = screen_w;
	if (r.y < 0) r.y = 0;
	if (r.y > screen_h) r.y = screen_h;

	// Das Zittern nur anwenden, wenn es berechnet wurde
	r.x += sway_x * dt * 10.0f;
	r.y += sway_y * dt * 10.0f;
}

void BowSimulation::draw_reticle(struct nk_context* ctx, const Reticle& r, float screen_w, float screen_h) {
	// Fenster-Styles komplett entfernen für ein reines Zeichen-Overlay
	nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
	nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0,0,0,0));
	nk_style_push_color(ctx, &ctx->style.window.border_color, nk_rgba(0,0,0,0));
	nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0, 0));
	nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));

	// Vollbild-Fenster öffnen, das keine Eingaben abfängt (Touches gehen durch an die View!)
	if (nk_begin(ctx, "HUD_Reticle", nk_rect(0, 0, screen_w, screen_h), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT)) {
		struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
		if (canvas) {
			// Farbe des Fadenkreuzes: Grün im Fokus, sonst Neon-Gelb/Rot
			struct nk_color color = (r.focus_radius <= r.min_focus + 2.0f) ? nk_rgb(0, 255, 0) : nk_rgb(255, 200, 0);

			// A) Äußerer Fokus-Kreis (zieht sich beim Spannen zusammen)
			nk_stroke_circle(canvas, nk_rect(r.x - r.focus_radius, r.y - r.focus_radius, r.focus_radius * 2, r.focus_radius * 2), 1.5f, color);

			// B) Das innere Fadenkreuz (Zentrierter Punkt)
			nk_fill_circle(canvas, nk_rect(r.x - 2, r.y - 2, 4, 4), nk_rgb(255, 255, 255));

			// C) Vier Zielstriche (Nord, Süd, West, Ost)
			float line_len = 10.0f;
			float gap = 6.0f; // Lücke zum Zentrum

			// Oben & Unten
			nk_stroke_line(canvas, r.x, r.y - gap - line_len, r.x, r.y - gap, 2.0f, color);
			nk_stroke_line(canvas, r.x, r.y + gap, r.x, r.y + gap + line_len, 2.0f, color);
			// Links & Rechts
			nk_stroke_line(canvas, r.x - gap - line_len, r.y, r.x - gap, r.y, 2.0f, color);
			nk_stroke_line(canvas, r.x + gap, r.y, r.x + gap + line_len, r.y, 2.0f, color);
		}
	}
	nk_end(ctx);

	nk_style_pop_vec2(ctx);
	nk_style_pop_vec2(ctx);
	nk_style_pop_color(ctx);
	nk_style_pop_color(ctx);
	nk_style_pop_style_item(ctx);
}