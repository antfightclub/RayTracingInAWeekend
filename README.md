# Ray Tracing in One Weekend
My adventures with following Peter Shirley, Trevor David Black, and Steve Hollasch's ray tracing books. 

Progress so far:
- [x] [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html)
- [x] [_Ray Tracing: The Next Week_](https://raytracing.github.io/books/RayTracingTheNextWeek.html)
- [x] [_Ray Tracing: The Rest of Your Life_](https://raytracing.github.io/books/RayTracingTheRestOfYourLife.html)

### Instructions on running
In Visual Studio Community 2022, open developer command prompt. Navigate to .\x64\Release or Debug. Then run
```bash
RayTracingInAWeekend.exe > image-title.ppm
```
This will create a .ppm file containing the result of the render. Open it in GIMP or ImageMagick.

### Project structure
Each book is in its own folder. Inside the book's folder is the source, as well as an Images folder which contains the output for each subsection.