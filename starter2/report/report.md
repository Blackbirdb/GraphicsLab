# 计算机图形学 Project 2 光照模型与光线追踪

> 黄宝岱 22307130480
>
> 秦雯钧 22300240024

## 任务一：Phong光照模型

### 任务要求

在 `Light.cpp`中填写 `PointLight::getIllumination()`方法，获取点光源对于给定点的相关信息；在 `Material.cpp`中填写 `Material::shade()`方法，返回漫反射强度和镜面反射强度之和；最后在 `Renderer::traceRay()`中完成 `Phong`光照模型的计算。

---

### 实现方法

#### PointLight::getIllumination()方法

按照文档中的提示信息完成 `tolight`，`intensity`和 `distToLight`的赋值即可。光强的计算需要用到以下公式：

$$
L ( x _ { \mathrm { s u r f } } ) = \frac { I } { \alpha d ^ { 2 } }
$$

#### Material::shade()方法

按照文档中的提示信息计算漫反射强度，和镜面反射强度,然后返回二者之和。其中完美反射矢量 `R`需要进行如下的计算：

```c++
    Vector3f R = (2.0f * Vector3f::dot(L, N) * N - L).normalized();
```

用到的公式如下：

$$
\text{clamp}(\mathbf{L}, \mathbf{N}) = 
\begin{cases} 
\mathbf{L} \cdot \mathbf{N} & \text{if } \mathbf{L} \cdot \mathbf{N} > 0 \\ 
0 & \text{otherwise} 
\end{cases}
$$

$$
I_{\text{diffuse}} = \text{clamp}(\mathbf{L} \cdot \mathbf{N}) \cdot L \cdot k_{\text{diffuse}}
$$

$$
I_{\text{specular}} = \text{clamp}(\mathbf{L} \cdot \mathbf{R})^s \cdot L \cdot k_{\text{specular}}
$$

#### Renderer::traceRay()方法

具体做法是先获取环境光信息，然后对 `_scene`中的所有光进行遍历获取光照信息，再调用之前实现的 `Material::shade()`方法计算该光对该物体造成的反射强度，和镜面反射强度之和，累加环境光，最后遍历结束获得的累加结果便是给定视线看到物体的颜色。

这个方法中，用到的公式如下：

$$
I_{\text{ambient}} =  L_{\text{ambient}} \cdot  k_{\text{diffuse}}
$$

$$
I = I_{\text{ambient}} + \sum_{i \in \text{lights}} \left( I_{\text{diffuse},i} + I_{\text{specular},i} \right)
$$

---

### 实验结果

<div style="page-break-after: always;"></div>

## 任务二：光线投射

### 任务要求

在 `Object3D.cpp`中填写 `Plane`、 `Triangle`、`Transform` 的 `Intersect()`函数的实现。

---

#### 平面类

因为该project中平面的表示是p*n = d，所以需要给平面类添加一个成员_p，用于记录平面内的一点，方便计算。完成初始化函数后，开始实现 `Intersect()`函数。

首先计算 `d * N`，然后判断 `d * N`是否为0，若为0，则说明视线与该平面平行，直接返回 `false`，不相交。然后根据文档提供的公式进行t的计算，若 `t < tmin` 或者 ` t >= h.getT()` 都返回 `false`，否则更新 `h`的 `t`值，返回 `true`。

用到的公式如下：

$$
t = \frac{(p' - o) \cdot N}{d \cdot N}
$$

#### 三角形类

首先获取三角形所在平面的法向量，两边的向量叉乘即可。

```c++
    Vector3f u = _v[1] - _v[0];
    Vector3f v = _v[2] - _v[0];
    Vector3f n = Vector3f::cross(u, v).normalized();
```

再获取平面的 `d`值，平面法向量点乘三角形任一点坐标即可。然后根据这些信息生成一个平面，再用之前实现的平面的 `Intersect()`函数判断是否相交，并且获取 `t`值。用这个 `t`值生成 `hit_point`，并且根据文档中的公式判断该点是否在三角形内，公式如下：

$$
P = (1 - u - v) A + u B + v C
$$

其中 `u`,`v`都在 `0-1`之间。若不满足点在三角形内的条件则返回 `false`，在的话需要计算该点的法向量，即：

```c++
Vector3f final_normal = param.x() * _normals[0] + param.y() * _normals[1] + param.z() * _normals[2];

```

最后与前面一样，若 `t`值小于 `h.getT()`则更新，返回 `true`。否则返回 `false`。

#### 变换类

首先为变换类添加一个4*4的矩阵成员，写好初始化方法。在 `intersect`函数的实现中，需要将视线原点和视线方向，变换到局部坐标系中。具体的方法便是将视线原点和视线方向齐次化后乘以 `Transform`类中矩阵的逆矩阵 `ray_local`，得到。然后将这个变换后的视线用于 `Transform`类成员 `object`的intersect函数，代码如下：

```c++
    if (_object->intersect(ray_local, tmin, my_hit) == false)
        return false;
```

因为计算的是局部坐标系的法向量，所以还需要将法向量通过变换转换为世界坐标系。这里用到了project 1的公式，将法向量齐次化以后乘以Transform类中矩阵的逆转置矩阵即可。公式如下：

$$
N' = \text{normalize}\left((M^{-1})^{T} N \right)
$$

值得注意的是 `t`值是无需变换的。因为变换前后光源到击中点的距离是不变的。最后对比所得的 `t`值和 `h.getT()`的大小，若小于则更新 `h`的 `t`值，返回 `true`；否则返回 `false`。


## 任务三：光线追踪与阴影投射

### 任务要求


通过在渲染器（Renderer::Render）递

归中进行函数调用来实现光线跟踪。你还可以通过投射阴影光线

来确定可见性


---

### 实验结果

截图等小黄同学做完我一起弄？
