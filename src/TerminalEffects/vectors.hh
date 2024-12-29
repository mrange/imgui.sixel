
struct vec1 {
  float x;

  explicit vec1()
    : x(0)
  {
  }


  explicit vec1(
      float x_
    )
    : x(x_)
  {
  }

  vec1& operator+=(float other) {
    x += other;

    return *this;
  }

  vec1 operator+(float other) const {
    vec1 c = *this;
    c += other;
    return c;
  }

  vec1& operator+=(vec1 const & other) {
    x += other.x;

    return *this;
  }

  vec1 operator+(vec1 const & other) const {
    vec1 c = *this;
    c += other;
    return c;
  }

  vec1& operator-=(float other) {
    x -= other;

    return *this;
  }

  vec1 operator-(float other) const {
    vec1 c = *this;
    c -= other;
    return c;
  }

  vec1& operator-=(vec1 const & other) {
    x -= other.x;

    return *this;
  }

  vec1 operator-(vec1 const & other) const {
    vec1 c = *this;
    c -= other;
    return c;
  }

  vec1& operator*=(float other) {
    x *= other;

    return *this;
  }

  vec1 operator*(float other) const {
    vec1 c = *this;
    c *= other;
    return c;
  }

  vec1& operator*=(vec1 const & other) {
    x *= other.x;

    return *this;
  }

  vec1 operator*(vec1 const & other) const {
    vec1 c = *this;
    c *= other;
    return c;
  }

  vec1& operator/=(float other) {
    x /= other;

    return *this;
  }

  vec1 operator/(float other) const {
    vec1 c = *this;
    c /= other;
    return c;
  }

  vec1& operator/=(vec1 const & other) {
    x /= other.x;

    return *this;
  }

  vec1 operator/(vec1 const & other) const {
    vec1 c = *this;
    c /= other;
    return c;
  }


  float dot(vec1 const & other) const {
    float sum = 0.F;

    sum += x*other.x;

    return sum;
  }


  float length() const {
    return std::sqrt(dot(*this));
  }

  void normalize__inplace() {
    *this *= 1.F/length();
  }

  vec1 normalized() const {
    vec1 c = *this;
    c.normalize__inplace();
    return c;
  }

  float distance(vec1 const & other) const {
    return (*this - other).length();
  }
};

struct vec2 {
  float x;
  float y;

  explicit vec2()
    : x(0)
    , y(0)
  {
  }

  explicit vec2(float c)
    : x(c)
    , y(c)
  {
  }

  explicit vec2(
      float x_
    , float y_
    )
    : x(x_)
    , y(y_)
  {
  }

  vec2& operator+=(float other) {
    x += other;
    y += other;

    return *this;
  }

  vec2 operator+(float other) const {
    vec2 c = *this;
    c += other;
    return c;
  }

  vec2& operator+=(vec2 const & other) {
    x += other.x;
    y += other.y;

    return *this;
  }

  vec2 operator+(vec2 const & other) const {
    vec2 c = *this;
    c += other;
    return c;
  }

  vec2& operator-=(float other) {
    x -= other;
    y -= other;

    return *this;
  }

  vec2 operator-(float other) const {
    vec2 c = *this;
    c -= other;
    return c;
  }

  vec2& operator-=(vec2 const & other) {
    x -= other.x;
    y -= other.y;

    return *this;
  }

  vec2 operator-(vec2 const & other) const {
    vec2 c = *this;
    c -= other;
    return c;
  }

  vec2& operator*=(float other) {
    x *= other;
    y *= other;

    return *this;
  }

  vec2 operator*(float other) const {
    vec2 c = *this;
    c *= other;
    return c;
  }

  vec2& operator*=(vec2 const & other) {
    x *= other.x;
    y *= other.y;

    return *this;
  }

  vec2 operator*(vec2 const & other) const {
    vec2 c = *this;
    c *= other;
    return c;
  }

  vec2& operator/=(float other) {
    x /= other;
    y /= other;

    return *this;
  }

  vec2 operator/(float other) const {
    vec2 c = *this;
    c /= other;
    return c;
  }

  vec2& operator/=(vec2 const & other) {
    x /= other.x;
    y /= other.y;

    return *this;
  }

  vec2 operator/(vec2 const & other) const {
    vec2 c = *this;
    c /= other;
    return c;
  }


  float dot(vec2 const & other) const {
    float sum = 0.F;

    sum += x*other.x;
    sum += y*other.y;

    return sum;
  }


  float length() const {
    return std::sqrt(dot(*this));
  }

  void normalize__inplace() {
    *this *= 1.F/length();
  }

  vec2 normalized() const {
    vec2 c = *this;
    c.normalize__inplace();
    return c;
  }

  float distance(vec2 const & other) const {
    return (*this - other).length();
  }
};

struct vec3 {
  float x;
  float y;
  float z;

  explicit vec3()
    : x(0)
    , y(0)
    , z(0)
  {
  }

  explicit vec3(float c)
    : x(c)
    , y(c)
    , z(c)
  {
  }

  explicit vec3(
      float x_
    , float y_
    , float z_
    )
    : x(x_)
    , y(y_)
    , z(z_)
  {
  }

  vec3& operator+=(float other) {
    x += other;
    y += other;
    z += other;

    return *this;
  }

  vec3 operator+(float other) const {
    vec3 c = *this;
    c += other;
    return c;
  }

  vec3& operator+=(vec3 const & other) {
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;
  }

  vec3 operator+(vec3 const & other) const {
    vec3 c = *this;
    c += other;
    return c;
  }

  vec3& operator-=(float other) {
    x -= other;
    y -= other;
    z -= other;

    return *this;
  }

  vec3 operator-(float other) const {
    vec3 c = *this;
    c -= other;
    return c;
  }

  vec3& operator-=(vec3 const & other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
  }

  vec3 operator-(vec3 const & other) const {
    vec3 c = *this;
    c -= other;
    return c;
  }

  vec3& operator*=(float other) {
    x *= other;
    y *= other;
    z *= other;

    return *this;
  }

  vec3 operator*(float other) const {
    vec3 c = *this;
    c *= other;
    return c;
  }

  vec3& operator*=(vec3 const & other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;

    return *this;
  }

  vec3 operator*(vec3 const & other) const {
    vec3 c = *this;
    c *= other;
    return c;
  }

  vec3& operator/=(float other) {
    x /= other;
    y /= other;
    z /= other;

    return *this;
  }

  vec3 operator/(float other) const {
    vec3 c = *this;
    c /= other;
    return c;
  }

  vec3& operator/=(vec3 const & other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;

    return *this;
  }

  vec3 operator/(vec3 const & other) const {
    vec3 c = *this;
    c /= other;
    return c;
  }


  float dot(vec3 const & other) const {
    float sum = 0.F;

    sum += x*other.x;
    sum += y*other.y;
    sum += z*other.z;

    return sum;
  }

  vec3 cross(vec3 const & other) const {
    return vec3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
  }

  float length() const {
    return std::sqrt(dot(*this));
  }

  void normalize__inplace() {
    *this *= 1.F/length();
  }

  vec3 normalized() const {
    vec3 c = *this;
    c.normalize__inplace();
    return c;
  }

  float distance(vec3 const & other) const {
    return (*this - other).length();
  }
};

struct vec4 {
  float x;
  float y;
  float z;
  float w;

  explicit vec4()
    : x(0)
    , y(0)
    , z(0)
    , w(0)
  {
  }

  explicit vec4(float c)
    : x(c)
    , y(c)
    , z(c)
    , w(c)
  {
  }

  explicit vec4(
      float x_
    , float y_
    , float z_
    , float w_
    )
    : x(x_)
    , y(y_)
    , z(z_)
    , w(w_)
  {
  }

  vec4& operator+=(float other) {
    x += other;
    y += other;
    z += other;
    w += other;

    return *this;
  }

  vec4 operator+(float other) const {
    vec4 c = *this;
    c += other;
    return c;
  }

  vec4& operator+=(vec4 const & other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;

    return *this;
  }

  vec4 operator+(vec4 const & other) const {
    vec4 c = *this;
    c += other;
    return c;
  }

  vec4& operator-=(float other) {
    x -= other;
    y -= other;
    z -= other;
    w -= other;

    return *this;
  }

  vec4 operator-(float other) const {
    vec4 c = *this;
    c -= other;
    return c;
  }

  vec4& operator-=(vec4 const & other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;

    return *this;
  }

  vec4 operator-(vec4 const & other) const {
    vec4 c = *this;
    c -= other;
    return c;
  }

  vec4& operator*=(float other) {
    x *= other;
    y *= other;
    z *= other;
    w *= other;

    return *this;
  }

  vec4 operator*(float other) const {
    vec4 c = *this;
    c *= other;
    return c;
  }

  vec4& operator*=(vec4 const & other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;

    return *this;
  }

  vec4 operator*(vec4 const & other) const {
    vec4 c = *this;
    c *= other;
    return c;
  }

  vec4& operator/=(float other) {
    x /= other;
    y /= other;
    z /= other;
    w /= other;

    return *this;
  }

  vec4 operator/(float other) const {
    vec4 c = *this;
    c /= other;
    return c;
  }

  vec4& operator/=(vec4 const & other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;

    return *this;
  }

  vec4 operator/(vec4 const & other) const {
    vec4 c = *this;
    c /= other;
    return c;
  }


  float dot(vec4 const & other) const {
    float sum = 0.F;

    sum += x*other.x;
    sum += y*other.y;
    sum += z*other.z;
    sum += w*other.w;

    return sum;
  }


  float length() const {
    return std::sqrt(dot(*this));
  }

  void normalize__inplace() {
    *this *= 1.F/length();
  }

  vec4 normalized() const {
    vec4 c = *this;
    c.normalize__inplace();
    return c;
  }

  float distance(vec4 const & other) const {
    return (*this - other).length();
  }
};


