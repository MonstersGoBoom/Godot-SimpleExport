# Godot-SimpleExport
export a scene from Godot into an easier to parse ascii format

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

note ResourceID of -1 indicates there's no 3D model for this Node. 





