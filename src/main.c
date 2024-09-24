#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <jni.h>

#include "android_native_app_glue.c"

#define LOG_ERR(...)  __android_log_print(ANDROID_LOG_ERROR, "MyApp", __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO,  "MyApp", __VA_ARGS__)
#define LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN,  "MyApp", __VA_ARGS__)

struct egl_state {
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
};

struct android_state {
	struct egl_state egl;
	GLuint vertex_buffer;
	GLint program;
	int initialized;
};

static int
egl_init(struct egl_state *egl, ANativeWindow *window)
{
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};

	egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(egl->display, NULL, NULL);

	EGLint config_count;
	eglChooseConfig(egl->display, attribs, &egl->config, 1, &config_count);
	egl->surface = eglCreateWindowSurface(egl->display, egl->config, window, NULL);

	EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl->context = eglCreateContext(egl->display, egl->config, NULL, context_attribs);
	if (!eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context)) {
		LOG_ERR("eglMakeCurrent");
		return -1;
	}

	return 0;
}

static void
egl_finish(struct egl_state *egl)
{
	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl->display, egl->context);
	eglDestroySurface(egl->display, egl->surface);
	eglTerminate(egl->display);
}

static const char *vertex_source =
	"precision mediump float;\n"
	"attribute vec2 v_pos;\n"
	"void main() {\n"
	"   gl_Position = vec4(v_pos.x, v_pos.y, 0.0, 1.0);\n"
	"}\n";

static const char *fragment_source =
	"precision mediump float;\n"
	"void main() {\n"
	"   gl_FragColor = vec4(1.0);\n"
	"}\n";

static float vertices[] = {
	-0.5f, 0.0f,
	 0.5f, 0.0f,
	 0.0f, 0.5f,
};

static void
handle_cmd(struct android_app *app, int32_t cmd)
{
	struct android_state *state = app->userData;
	switch (cmd) {
	case APP_CMD_INIT_WINDOW:
		if (app->window) {
			egl_init(&state->egl, app->window);

			GLint success = 0;
			glGenBuffers(1, &state->vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertex_shader, 1, &vertex_source, NULL);
			glCompileShader(vertex_shader);
			glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char info_log[1024];
				glGetProgramInfoLog(vertex_shader, sizeof(info_log), NULL, info_log);
				LOG_ERR("vertex shader: %s\n", info_log);
			}

			GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment_shader, 1, &fragment_source, NULL);
			glCompileShader(fragment_shader);
			glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char info_log[1024];
				glGetProgramInfoLog(fragment_shader, sizeof(info_log), NULL, info_log);
				LOG_ERR("fragment shader: %s\n", info_log);
			}

			state->program = glCreateProgram();
			glAttachShader(state->program, vertex_shader);
			glAttachShader(state->program, fragment_shader);
			glLinkProgram(state->program);

			glGetProgramiv(state->program, GL_LINK_STATUS, &success);
			if (!success) {
				char info_log[1024];
				glGetProgramInfoLog(state->program, sizeof(info_log), NULL, info_log);
				LOG_ERR("%s\n", info_log);
			}

			state->initialized = 1;
		}
		break;
	}
}

static int32_t
handle_input(struct android_app *app, AInputEvent *event)
{
	switch (AInputEvent_getType(event)) {
	case AINPUT_EVENT_TYPE_KEY:
		break;
	case AINPUT_EVENT_TYPE_MOTION:
		break;
	case AINPUT_EVENT_TYPE_FOCUS:
		break;
	case AINPUT_EVENT_TYPE_CAPTURE:
		break;
	case AINPUT_EVENT_TYPE_DRAG:
		break;
	case AINPUT_EVENT_TYPE_TOUCH_MODE:
		break;
	}

	return 0;
}

void
android_main(struct android_app *app)
{
	struct android_state state = {0};
	app->userData = &state;
	app->onAppCmd = handle_cmd;
	app->onInputEvent = handle_input;

	int events;
	struct android_poll_source *source;
	do {
		if (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
			if (source) {
				source->process(app, source);
			}
		}

		if (state.initialized) {
			int32_t height = ANativeWindow_getHeight(app->window);
			int32_t width = ANativeWindow_getWidth(app->window);
			glViewport(0, 0, width, height);
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(state.program);
			glBindBuffer(GL_ARRAY_BUFFER, state.vertex_buffer);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			eglSwapBuffers(state.egl.display, state.egl.surface);
		}
	} while (!app->destroyRequested);

	egl_finish(&state.egl);
}
