# Third-party code

## glad

`glad/` is an OpenGL 3.3 core-profile function loader generated with [glad2](https://github.com/Dav1dde/glad). It is committed so the build needs no Python step. To regenerate it (for example to target a newer OpenGL version):

```sh
pip install glad2
python -m glad --api gl:core=3.3 --out-path third_party/glad --reproducible c
```

GLFW and GLM are not vendored; CMake downloads pinned versions with `FetchContent` at configure time.
