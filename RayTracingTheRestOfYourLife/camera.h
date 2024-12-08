#ifndef CAMERA_H
#define CAMERA_H

#include <chrono>
#include <time.h>
#define  _CRT_SECURE_NO_WARNINGS
#include "Timer.h"
#include "hittable.h"
#include "pdf.h"
#include "material.h"

class camera {
public:
	double aspect_ratio      = 1.0; // Ratio of image width over height
	int    image_width       = 100; // Pixels
	int    samples_per_pixel = 10;  // Count of random samples for each pixel 
	int    max_depth         = 10;  // Maximum number of ray bounces into scene
	color  background;				// Scene background color

	double vfov     = 90; // Vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, 0);
	point3 lookat   = point3(0, 0, -1);
	vec3   vup      = vec3(0,1,0);

	double defocus_angle = 0;
	double focus_dist = 10;

	void render(const hittable& world, const hittable& lights) {
		Timer t;
		initialize();
		

		std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

		for (int j = 0; j < image_height; j++) {
			std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
			for (int i = 0; i < image_width; i++) {
				color pixel_color(0, 0, 0);
				for (int s_j = 0; s_j < sqrt_spp; s_j++) {
					for (int s_i = 0; s_i < sqrt_spp; s_i++) {
						ray r = get_ray(i, j, s_i, s_j);
						pixel_color += ray_color(r, max_depth, world, lights);
					}
				}
				write_color(std::cout, pixel_samples_scale * pixel_color);
			}
		}

		std::clog << "\rDone.                       \n" <<
			"Seconds taken: " << t.elapsed() << " seconds\n"
			<< " Hours  taken: " << (t.elapsed() / 3600) << " hours";
	}

private:
	int    image_height;
	double pixel_samples_scale;  // Color scale factor for a sum of pixel samples.
	int	   sqrt_spp;			 // Square root of number of samples per pixel
	double recip_sqrt_spp;		 // 1 / sqrt_spp
	point3 center;
	point3 pixel00_loc;
	vec3   pixel_delta_u;
	vec3   pixel_delta_v;
	vec3   u, v, w;              // Camera frame basis vectors
	vec3   defocus_disk_u;
	vec3   defocus_disk_v;

	void initialize() {
		image_height = int(image_width / aspect_ratio);
		image_height = (image_height < 1) ? 1 : image_height;

		sqrt_spp = int(std::sqrt(samples_per_pixel));
		pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
		recip_sqrt_spp = 1.0 / sqrt_spp;

		center = lookfrom;

		// Determine viewport dimensions
		auto theta = degrees_to_radians(vfov);
		auto h = std::tan(theta / 2);
		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * (double(image_width) / image_height);

		// Calculate the u,v,w unit basis vectors for the camera coordinate frame
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);

		// Calculate the vectors across the horizontal and down the vertical viewport edges
		auto viewport_u = viewport_width * u;
		auto viewport_v = viewport_height * -v;

		// Calculate the horizontal and vertical delta vectors from pixel to pixel
		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		// Calculate the location of the upper left pixel
		auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		// Calculate the camera defocus disk basis vectors
		auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}

	ray get_ray(int i, int j, int s_i, int s_j) const {
		// Construct a camera ray originating from the defocus disk and directed at randomly sampled
		// point around the pixel location i, j.

		auto offset = sample_square_stratified(s_i, s_j);
		auto pixel_sample = pixel00_loc
			+ ((i + offset.x()) * pixel_delta_u)
			+ ((j + offset.y()) * pixel_delta_v);

		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto ray_direction = pixel_sample - ray_origin;
		auto ray_time = random_double();

		return ray(ray_origin, ray_direction, ray_time);
	}

	vec3 sample_square_stratified(int s_i, int s_j) const {
		// Returns the vector to a random point in the square sub-pixel specified by grid
		// indices s_i and s_j, for an idealized unit square pixel [-.5, -.5] to [+.5, +.5]

		auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
		auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

		return vec3(px, py, 0);
	}

	vec3 sample_square() const {
		// Returns the vector to a random point in the [-.5, -.5]-[+.5, +.5] unit square
		return vec3(random_double() - 0.5, random_double() - 0.5, 0);
	}

	point3 defocus_disk_sample() const {
		// Returns a random point in the camera defocus disk
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	color ray_color(const ray& r, int depth, const hittable& world, const hittable& lights) const {
		// If we've exceeded the ray bounce limit, no more light is gathered
		if (depth <= 0)
			return color(0, 0, 0);

		hit_record rec;

		// If the ray hits nothing, return the background color
		if (!world.hit(r, interval(0.001, infinity), rec))
			return background;

		ray scattered;
		color attenuation;
		double pdf_value;
		color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

		if (!rec.mat->scatter(r, rec, attenuation, scattered, pdf_value))
			return color_from_emission;

		hittable_pdf light_pdf(lights, rec.p);
		scattered = ray(rec.p, light_pdf.generate(), r.time());
		pdf_value = light_pdf.value(scattered.direction());

		double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);
		
		color sample_color = ray_color(scattered, depth - 1, world, lights);
		color color_from_scatter = (attenuation * scattering_pdf * sample_color) / pdf_value;

		return color_from_emission + color_from_scatter;
	}
};

#endif