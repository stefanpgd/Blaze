#include "Framework/Blaze.h"

// TODO Summary:
// I want to turn Blaze from a Path Tracer to more of a DXR Engine, similary to snowdrop.
// This means that I should have different rendering stages. 

// Features/Systems I want to make:
// - A 'Diffuse' ray traced render
// - Texture support
// - Ray Traced Shadows
// - Ray Traced Reflections
// - A mode to turn on full on path tracing in the scene
// - Implement Ray Cone Tracing for first hit 

// Some things worth investigating?
// Probability density functions
// Denoising Techniques (Reprojection?)

int main()
{
	Blaze app;
	app.Run();

	return 0;
}