#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

SDL_Window *window;
SDL_GLContext context;
GLuint vertexShader, fragmentShader;

typedef char *Text;

enum {
	ELOAD = -1,
};

void Panic(int code, const char *format, ...) {
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);

  exit(code);
}
static const char *ErrorString(GLenum error) {
  switch (error) {
    case GL_NO_ERROR: return "GL_NO_ERROR"; break;
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM"; break;
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE"; break;
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY"; break;
  }

  return NULL;
}

void Panic_on_glGetError(const char *tag) {
  GLenum error = glGetError();
  if (error) {
    printf("%s panic: ", tag);
    Panic(1, "%s (%d)\n", ErrorString(error), error);
  }
}

Text Text_New(const char *format, ...) {
	va_list size_args, format_args;
	va_start(format_args, format);
	va_copy(size_args, format_args);

	int size = 1 + vsnprintf(NULL, 0, format, size_args);
	va_end(size_args);

	Text t = (Text) calloc(size, sizeof(char));
	vsnprintf (t, size, format, format_args);
	va_end(format_args);

	return t;
}

Text Text_FromFile(const char *path) {
	FILE *file = fopen(path, "r");
	if (!file) {
		Panic(ELOAD, "Text_FromFile: unable to find '%s'", path);
	}

	long size = 0;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	Text buffer = (Text) calloc(size+1, sizeof(char));
	if (!buffer) Panic(ELOAD, "Text_FromFile: unable to allocate buffer for '%s'\n", path);

	if(fread(buffer, 1, size, file) != size) Panic(ELOAD, "Text_FromFile: error reading '%s'\n", path);

	fclose(file);
	return buffer;
}

Text Text_Free(Text t) {
	if(t) free(t);
	return NULL;
}

bool Window_New() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  window = SDL_CreateWindow("OpenGL", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
  context = SDL_GL_CreateContext(window);

  glewExperimental = GL_TRUE;
  glewInit();

  return true;
}

void Window_Destroy() {
  SDL_GL_DeleteContext(context);
  SDL_Quit();
}

bool Window_Event() {
  SDL_Event Window_Event;
  while (true) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw a rectangle from the 2 triangles using 6 indices
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    if (SDL_PollEvent(&Window_Event)) {
      if (Window_Event.type == SDL_QUIT) return false;

      if (Window_Event.type == SDL_KEYUP && Window_Event.key.keysym.sym == SDLK_ESCAPE) return false;

      SDL_GL_SwapWindow(window);
    }
  }

    return true;
}

GLuint Shader_FromFile(const char *filename, GLenum shaderType) {
  const char *src = Text_FromFile(filename);

  GLuint shader = glCreateShader(shaderType);

  glShaderSource(shader, 1, &src, NULL);
  Text_Free((Text) src);

  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

  if (!status) {
    char log[512];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    Panic(1, log);
  }

  return shader;
}

GLuint ShaderProgram_New(const char *filename, GLuint *shader, GLenum shaderType, ...) {
  GLuint program = glCreateProgram();

  va_list args;
  va_start(args, shaderType);

  while (filename && shaderType) {
    *shader = Shader_FromFile(filename, shaderType);
    glAttachShader(program, *shader);

    filename = (const char *) va_arg(args, const char *);
    shader = va_arg(args, GLuint *);
		shaderType = va_arg(args, GLenum);
  }
  va_end(args);

  Panic_on_glGetError("ShaderProgram_New");
  return program;
}

#define PROGRAM_NEW(...) ShaderProgram_New(__VA_ARGS__, NULL, NULL, 0)

GLuint Vao_New() {
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  Panic_on_glGetError("Vao_New");
  return vao;
}

GLuint GlBuffer_New(GLenum target, GLsizeiptr size, const GLvoid *data) {
  GLuint buffer;

  glGenBuffers(1, &buffer); // Generate 1 buffer
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, GL_STATIC_DRAW);

  return buffer;
}

GLuint Vbo_New(GLsizeiptr size, const GLvoid *data) {
  GLuint vbo = GlBuffer_New(GL_ARRAY_BUFFER, size, data);
  Panic_on_glGetError("Vbo_New");

  return vbo;
}

GLuint Ebo_New(GLsizeiptr size, const GLvoid *data) {
  GLuint ebo = GlBuffer_New(GL_ELEMENT_ARRAY_BUFFER, size, data);
  Panic_on_glGetError("Ebo_New");

  return ebo;
}

GLint ShaderProgram_SetAttrib(GLuint program, const GLchar *name, GLint size, GLsizei stride, unsigned long offset) {
  GLint index = glGetAttribLocation(program, name);
  glEnableVertexAttribArray(index);

  stride *=  sizeof(GLfloat);

  unsigned long pointer = offset * sizeof(GLfloat);
  glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, (const GLvoid *) pointer);

  Panic_on_glGetError("ShaderProgram_SetAttrib");
  return index;
}

typedef struct ShaderProgram {
  ;
  unsigned long offset;
} ShaderProgram;

static void GetAttribFromArgs(va_list args, const GLchar **name, GLint **attrib, GLint *size) {
  *name = (const char *) va_arg(args, const char *);
  *attrib = (GLint *) va_arg(args, GLint *);
  *size = (GLint) va_arg(args, GLint);
}

static GLsizei GetStride(va_list src, GLsizei size) {
  va_list args;
  va_copy(args, src);

  const GLchar *name;
  GLint *attrib, stride = 0;

  do {
    stride += size;

    GetAttribFromArgs(args, &name, &attrib, &size);
  } while(name && attrib && size);

  va_end(args);

  return stride;
}

void ShaderProgram_SetAttribs(GLuint program, const GLchar *name, GLint *attrib, GLint size, ...) {
  if (!(program && name && attrib && size)) Panic(1, "ShaderProgram_SetAttribs: args not set");

  va_list args;
  va_start(args, size);

  GLsizei stride = GetStride(args, size);
  printf("%d\n", stride);
  unsigned long offset = 0;

  while(name && attrib && size) {
    *attrib = ShaderProgram_SetAttrib(program, name, size, stride, offset);
    offset += size;
    GetAttribFromArgs(args, &name, &attrib, &size);
  }

  va_end(args);
}

int main(int argc, char *argv[]) {
  Window_New();

  GLfloat vertices[] = {
          -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // Top-left
           0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // Top-right
           0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // Bottom-right
          -0.5f, -0.5f, 1.0f, 1.0f, 1.0f  // Bottom-left
  };

  GLuint vao = Vao_New();
  GLuint vbo = Vbo_New(sizeof(vertices), vertices);

  GLuint elements[] = {
      0, 1, 2,
      2, 3, 0
  };

  GLuint ebo = Ebo_New(sizeof(elements), elements);

  ShaderProgram sp;

  GLuint program = ShaderProgram_New(
    "./shader.vert", &vertexShader, GL_VERTEX_SHADER,
    "./shader.frag", &fragmentShader, GL_FRAGMENT_SHADER,
    NULL, NULL, 0
  );

  glBindFragDataLocation(program, 0, "outColor");
  glLinkProgram(program);
  glUseProgram(program);
  Panic_on_glGetError("link/use program");

  // Specify the layout of the vertex data
  GLint position;
  GLint color;
  ShaderProgram_SetAttribs(program,
    "position", &position, 2,
    "color", &color, 3,
    NULL, NULL, 0
  );

  Window_Event();

  Window_Destroy();
  return 0;
}
