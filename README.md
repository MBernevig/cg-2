# cg-2 Tech Demo Assignment

Authors: Mihnea Bernevig, Codrin Ogreanu


- Multiple viewpoints (Mihnea 100%)
- Normal mapping (Codrin 100%)
- Environment mapping (Mihnea 100%)
- Smooth paths (Codrin 100%)
- Hierarchical Transformations (Mihnea 100%)
- Material Textures (Mihnea 50%, Codrin 50%)
- Minimap (Mihnea 100%)
- Shadows from multiple light sources (Codrin 100%)
- Animated textures (Mihnea 100%)
- Day-Night system (Mihnea 100%)

## Features Implemented: 

1. Multiple viewpoints: 
   
   Our project features multiple viewpoints for the user to choose from, namely a first person fly camera, a third person camera following the player character, and a minimap camera that follows the player from above.
   The user can switch between each camera and also control various parameters for them, such as follow distance, offset and height for third person, and height for the minimap camera.

   First person view also omits rendering the character model to avoid clipping issues.

* First person view
<img title="First person view" alt="Alt text" src="./report_screenshots/fpv.png">
* Third person view
<img title="a title" alt="Alt text" src="./report_screenshots/tpv.png">
* Minimap camera view
<img title="a title" alt="Alt text" src="./report_screenshots/minimapcam.png">

2. Normal mapping.

    We also implemented normal mapping using a normal texture. This normal texture loads automatically using information stored in the .mtl file of the mesh.

    ![alt text](image.png)

    ![alt text](image-1.png)

3. Environment mapping
   
   We implement a skybox that always sits at depth 1, using a cubemap texture. To showcase environment mapping we also created a cube that is either reflective or refractive. The following youtube video offered great insight into this feature: [Victor Gordan - OpenGL Tutorial 19 - Cubemaps & Skyboxes, https://www.youtube.com/watch?v=8sVvxeKI9Pk](https://www.youtube.com/watch?v=8sVvxeKI9Pk). The skybox texture is also used to approximate ambient lighting, using the fragment's normal value and sampling the skybox cubemap based on it.
   
   Reflective cube and skybox texture: ![alt text](image-2.png)

   Refractive cube, with refraction idex of 1.55: ![alt text](image-3.png)


4. Smooth paths: 
    We implemented a light moving along a bezier curve. 

    ![alt text](image-6.png)

    ![alt text](image-7.png)

5. Hierarchical Transformations:
   
   Similarly to the first CG assignment, we set up a parent-child relation between meshes, updating the children's model matrix with their parents'. The following crude hatchet was created to showcase this feature:

   ![alt text](image-4.png)

   ![alt text](image-5.png)

6. Material Textures:
   
   We have implemented only Kd textures for this demo:

   ![alt text](image-8.png)

   ![alt text](image-9.png)

7. Minimap

    Pressing the M key, or toggling the feature via the UI will open up a minimap. This is simply a view from the aforementioned minimap camera, rendered to a texture and then displayed on a quad. An orthographic projection is used to render the texture. The minimap only renders objects belowe the player position. This is done so that the player's current level is always visible.

    ![alt text](image-10.png)

    ![alt text](image-11.png)

8. Shadows from multiple light sources
   
   Our system can render shadows from up to 10 different light sources. It draws a different shadow map for each light source, and blends everything together in the shader.

   ![alt text](image-12.png)

   ![alt text](image-13.png)

9. Animated textures
    
    We rendered 20 different images from a blender animation, and then displayed these on a screen mesh we added to the scene. To ensure the animation is independent of the framerate, we use a 60 tick-per-second update rate, with a frametime accumulator. This ensures that the current frame is always accurate to what should be displayed.

    ![alt text](image-14.png)

    ![alt text](image-15.png)

10. Day-Night system
    
    We update the skybox texture every 10 seconds to switch from day to night. Since the ambient color is also sampled from the skybox itself, this gives the effect that the time of day is actually changing. This also makes use of the 60 tps update method.

    Night:
    ![alt text](image-16.png)

    Dusk: 
    ![alt text](image-17.png)