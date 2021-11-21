#include <cstdio>
#include <cmath>
#include <cstdint>
#include <limits>
#include <math.h>
#include <vector>
#include <variant>
#include <optional>

struct Vec3
{
  float x, y, z;

  Vec3() : x(0), y(0), z(0) { }
  Vec3(float x, float y, float z) : x(x), y(y), z(z) { }

public: // ops
  float length2() const {
    return x*x + y*y + z*z;
  }

  float length() const {
    return sqrt(length2());
  }

  Vec3 normalize() const {
    float l = length();
    if(l == 0)
      return *this;
    return (*this) * (1.0 / l);
  }

  // https://en.wikipedia.org/wiki/Dot_product
  float dot(Vec3 other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  // https://en.wikipedia.org/wiki/Cross_product
  Vec3 cross(Vec3 other) const {
    return Vec3 {
      y * other.z - z * other.y,
      z * other.x - x * other.z,
      x * other.y - y * other.x,
    };
  }

  // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/reflect.xhtml
  Vec3 reflect(Vec3 normal) const {
    return *this - normal * 2.0 * normal.dot(*this);
  }

public: // operators
 Vec3 operator *(float s) const {
    return Vec3 {
      x * s,
      y * s,
      z * s,
    };
  }

  Vec3 operator +(Vec3 other) const {
    return Vec3 {
      x + other.x,
      y + other.y,
      z + other.z,
    };
  }

  Vec3 operator -(Vec3 other) const {
    return Vec3 {
      x - other.x,
      y - other.y,
      z - other.z,
    };
  }

  Vec3 operator-() const {
    return Vec3 {-x,-y,-z};
  }
};

struct Color 
{
  float r, g, b;

  Color() : Color(0.0) { }
  Color(float w) : Color(w,w,w) { }
  Color(float r, float g, float b) : r(r), g(g), b(b) { }

  float brightness() const {
    return 0.299 * r + 0.587 * g + 0.114 * b;
  }

public: // operators
  Color operator *(float s) const {
    return Color {
      r * s,
      g * s,
      b * s,
    };
  }

  Color operator *(Color other) const {
    return Color {
      r * other.r,
      g * other.g,
      b * other.b,
    };
  }

  Color operator /(Color other) const {
    return Color {
      r / other.r,
      g / other.g,
      b / other.b,
    };
  }

  Color operator +(Color other) const {
    return Color {
      r + other.r,
      g + other.g,
      b + other.b,
    };
  }

  Color operator -(Color other) const {
    return Color {
      r - other.r,
      g - other.g,
      b - other.b,
    };
  }

  Color & operator += (Color other) {
    *this = *this + other;
    return *this;
  }

  Color & operator *= (Color other) {
    *this = *this * other;
    return *this;
  }
};

struct Image
{
  size_t width, height;
  std::vector<Color> pixels;

  Image(size_t width, size_t height) :
    width(width), height(height), pixels(width * height)
  {

  }

  void clear(Color color) 
  {
    for(Color & c : pixels) {
      c = color;
    }
  }

  template<typename F>
  void apply(F const & f) 
  {
    for(Color & c : pixels) {
      c = f(c);
    }
  }

  Color & at(size_t x, size_t y) {
    return this->pixels[y * width + x];
  }

  Color get(size_t x, size_t y) const {
    return this->pixels[y * width + x];
  }

  void set(size_t x, size_t y, Color color) {
    this->pixels[y * width + x] = color;
  }

  bool save(char const * file_name) const 
  {
    FILE * f = fopen(file_name, "wb");
    if(f == nullptr)
      return false;
    
    fprintf(f, "P6 %lu %lu 255\n", width, height);

    for(Color c : pixels)
    {
      uint8_t binary[3] = {
        uint8_t(std::max(0.0, std::min(255.0, 255.0 * c.r))),
        uint8_t(std::max(0.0, std::min(255.0, 255.0 * c.g))),
        uint8_t(std::max(0.0, std::min(255.0, 255.0 * c.b))),
      };
      fwrite(binary, 3, 1, f);
    }

    fclose(f);
    return true;
  }
};

struct Camera
{
  Vec3 position;
  Vec3 forward;
  Vec3 right;
  float focal_length = 1.0;

  void lookAt(Vec3 pos, Vec3 dest, Vec3 up)
  {
    this->position = pos;
    this->forward = (dest - pos).normalize();
    this->right = up.cross(this->forward).normalize();
  }

  Vec3 projectRay(float x, float y) const
  {
    return (this->right * x + this->forward.cross(this->right) * y + this->forward * focal_length).normalize();
  }
};

struct Material
{
  Color albedo;
  float reflectivity;
};

struct PointLight
{
  Vec3 position;
  float power;
  Color color;
};


struct Intersection
{
  float distance;
  Vec3 position;
  Vec3 normal;
  Material const * material;
};

struct Plane
{
  Material * const material;
  Vec3 origin;
  Vec3 normal;
  
  // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
  std::optional<Intersection> intersect(Vec3 ray_origin, Vec3 ray_direction) const 
  {
    // assuming vectors are all normalized
    float denom = -normal.dot(ray_direction); 
    if (denom > 1e-6) { 
        Vec3 p0l0 = origin - ray_origin; 
        float t = -p0l0.dot(normal) / denom; 
        if(t >= 0) {
          return Intersection {
            t,
            ray_origin + ray_direction * t,
            normal,
            material,
          };
        }
    } 
    return std::nullopt;
  }
};

struct Sphere 
{
  Material * const material;
  Vec3 center;
  float radius;

  // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
  std::optional<Intersection> intersect(Vec3 ray_origin, Vec3 ray_direction) const 
  {
    float radius2 = radius * radius;
    
    Vec3 L = center - ray_origin; 
    float tca = L.dot(ray_direction); 
    float d2 = L.dot(L) - tca * tca;
    if (d2 > radius2) {
      return std::nullopt; 
    }
    float thc = sqrt(radius2 - d2); 
    float t0 = tca - thc; 
    float t1 = tca + thc; 

    if (t0 > t1) {
      std::swap(t0, t1); 
    }

    if (t0 < 0) { 
        t0 = t1; // if t0 is negative, let's use t1 instead 
        if (t0 < 0) {
          return std::nullopt; // both t0 and t1 are negative 
        }
    } 

    return Intersection {
      t0,
      ray_origin + ray_direction * t0,
      (ray_origin + ray_direction * t0 - center).normalize(),
      material,
    }; 
  }
};

using Object = std::variant<Plane, Sphere>;


struct Scene
{
  std::vector<Object> objects;
  std::vector<PointLight> lights;

  std::optional<Intersection> intersect(Vec3 ray_origin, Vec3 ray_direction) const 
  {
    Intersection final_hit;
    final_hit.distance = std::numeric_limits<float>::max();

    for(Object const & obj : objects)
    {
      auto hit = std::visit([=](auto & obj) -> std::optional<Intersection> {
        return obj.intersect(ray_origin, ray_direction);
      }, obj);
      if(hit != std::nullopt) {
        if(hit->distance < final_hit.distance) {
          final_hit = *hit;
        }
      }
    }

    if(final_hit.distance != std::numeric_limits<float>::max()) {
      return final_hit;
    } else {
      return std::nullopt;
    }
  }

  static constexpr size_t max_recursion = 10;
  std::optional<Color> trace(Vec3 ray_origin, Vec3 ray_direction, size_t recursion = max_recursion) const 
  {
      auto intersection = intersect(ray_origin, ray_direction);
      if(intersection == std::nullopt)
        return std::nullopt;
      
      Material const * surface_mtl = intersection->material;

      Color surface_albedo = surface_mtl->albedo;
      Color surface_reflection { 0.0 };

      if(surface_albedo.brightness() > 0.0)
      {
        Color lighting { 0.1 }; // fake some basic ambient lighting
        for(auto const & light : lights)
        {
          Vec3 light_delta = (intersection->position - light.position);

          float distance_to_light = light_delta.length();
          if(auto hit = intersect(light.position, light_delta.normalize()))
          {
            if(hit->distance < (distance_to_light - 1e-3)) { // needs tiny delta due to imprecision
              // ray to light is obstructed
              continue;
            }
          }

          float attenuation = light.power / distance_to_light;

          lighting += light.color * attenuation;
        }
        surface_albedo *= lighting;
      }

      // these things might have recursion, guard them
      if(recursion > 0)
      {
        if(surface_mtl->reflectivity > 0.0)
        {
          Vec3 refl_dir = ray_direction.reflect(intersection->normal);
          Vec3 refl_origin = intersection->position + refl_dir * 1e-4;

          if(auto hit = trace(refl_origin, refl_dir, recursion - 1))
          {
            surface_reflection = *hit;
          }
        }
      }

      return surface_albedo + surface_reflection;
  }

};

int main()
{
  Image target { 512, 512 };
  target.clear(Color(0,0,0));

  Camera camera;
  camera.lookAt(
    Vec3(0,0,-10), 
    Vec3(0,0,0), 
    Vec3(0,1,0)
  );

  Material left_plane { Color(1,0,0), 0.0 };
  Material right_plane { Color(0,1,0), 0.0 };
  Material other_plane { Color(0.8), 0.0 };
  Material mirror_sphere { Color(0.0), 1.0 };

  Scene scene;
  // cornell box
  scene.objects.push_back(Object { Plane { &left_plane, Vec3 { -10, 0, 0  }, Vec3 { 1, 0, 0  } } });
  scene.objects.push_back(Object { Plane { &right_plane, Vec3 {  10, 0, 0  }, Vec3 { -1, 0, 0  } } });
  scene.objects.push_back(Object { Plane { &other_plane, Vec3 {  0, -10, 0  }, Vec3 { 0, 1, 0  } } });
  scene.objects.push_back(Object { Plane { &other_plane, Vec3 {  0, 10, 0  }, Vec3 { 0, -1, 0  } } });
  scene.objects.push_back(Object { Plane { &other_plane, Vec3 {  0, 0, 10  }, Vec3 { 0, 0, -1  } } });

  // some spheres
  scene.objects.push_back(Object { Sphere { &mirror_sphere, Vec3 { 0, -5, -5 }, 2.0 } });
  scene.objects.push_back(Object { Sphere { &mirror_sphere, Vec3 { 4.33, -4, 2.5 }, 2.0 } });
  scene.objects.push_back(Object { Sphere { &mirror_sphere, Vec3 { -4.33, -4.5, 2.5 }, 2.0 } });
  
  // some lights
  scene.lights.push_back(PointLight { Vec3{0,0,0}, 10.0f, Color{1,1,1} });

  for(size_t y = 0; y < target.height; y++)
  {
    for(size_t x = 0; x < target.width; x++)
    {
      float ss_x = 2.0 * float(x) / float(target.width - 1) - 1.0;
      float ss_y = 1.0 - 2.0 * float(y) / float(target.height - 1);

      Vec3 ray_origin = camera.position;
      Vec3 ray_direction = camera.projectRay(ss_x, ss_y);

      if(auto color = scene.trace(ray_origin, ray_direction))
      {
        target.set(x, y, *color);
      }
      else 
      {
        target.set(x, y, Color(0));
      }
    }
  }

  // apply basic color grading
  // see: https://learnopengl.com/Advanced-Lighting/HDR

  // target.apply([](Color c) -> Color 
  // {
  //   // reinhard tone mapping
  //   return c / (c + Color(1.0));
  // });

  // float exposure = 1.00;
  // target.apply([exposure](Color c) -> Color 
  // {
  //   // exposure tone mapping
  //   return Color(1.0) - Color(exp(-c.r * exposure), exp(-c.g * exposure), exp(-c.b * exposure));
  // });

  // apply gamma correction
  float gamma = 2.2;
  target.apply([gamma](Color c) -> Color 
  { 
    return Color {
      powf(c.r, 1.0f / gamma),
      powf(c.g, 1.0f / gamma),
      powf(c.b, 1.0f / gamma),
    };

  });

  target.save("output.pgm");
  return 0;
}