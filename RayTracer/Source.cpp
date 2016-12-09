#include "include\GLFW\glfw3.h"

#include "Scene.h"
#include "RayTracer.h"
#include "PerformanceTester.h"
int width = 1280;
int height = 720;
Scene* mainscene;
RayTracer* tracer;
PerformanceTester* tester;
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_F1:
			tracer->m_traceflag = RayTracer::TRACE_AMBIENT;
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F2:
			tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC);
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F3:
			tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC
				| RayTracer::TRACE_SHADOW);
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F4:
			tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC
				| RayTracer::TRACE_REFLECTION | RayTracer::TRACE_SHADOW);
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F5:
			tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC
				| RayTracer::TRACE_REFRACTION);
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F6:
			tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC
				| RayTracer::TRACE_REFRACTION | RayTracer::TRACE_REFLECTION | RayTracer::TRACE_SHADOW);
			tracer->ResetRenderCount();
			break;
		case GLFW_KEY_F8:
			//run the performance test;
			tester->RunTests();
			break;
		case GLFW_KEY_F9:
			//run the performance test;
			tester->RunAATests();
			break;
		default:
			break;
		}
	}

}
void init() {
	mainscene = new Scene();
	mainscene->InitDefaultScene();
	mainscene->SetSceneWidth((float)width / (float)height);
	tracer = new RayTracer(width, height);
	tester = new PerformanceTester(tracer, mainscene);
}
int main(void)
{
	init();
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "RayTracer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);
	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	
	while (!glfwWindowShouldClose(window))
	{
		if (tracer->GetCanRender()) {
			/* Render here */
			glClear(GL_COLOR_BUFFER_BIT);

			tracer->RayTrace(mainscene);
			Colour *pBuffer = tracer->GetFramebuffer()->GetBuffer();
			glDrawPixels(width, height, GL_RGB, GL_FLOAT, pBuffer);

			glFlush();

			/* Swap front and back buffers */
			glfwSwapBuffers(window);
		}
		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}