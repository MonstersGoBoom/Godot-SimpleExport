# Godot-SimpleExport
Export a scene from Godot into an easier to parse ascii format

Copy into your Godot project. ( keep addons/ascii_export filepath ) 

file contains one line per "thing" 

first Resources. which are simply pathnames for 3d models 

```
resources:<int>
filename.obj
house.gltf
.....
```

then 

```
nodes:<int>
ClassName , NodeName , ParentName , Resource ID , 3*4 floats for matrix

eg. 
MeshInstance3D,rock_B,rock_B2,37,1,0,0,0,1,0,0,0,1,0,0,0
```

NOTES:

ResourceID of -1 indicates there's no 3D model for this Node. 

all matrices are in local space. 
when rendering you need to reference the parent node worldMatrix and multiply the local and parent world together. 

will post a simple example.








