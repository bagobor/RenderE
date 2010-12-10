/* 
 * File:   Mesh.h
 * Author: morten
 *
 * Created on November 12, 2010, 8:51 PM
 */

#ifndef MESH_COMPONENT_H
#define	MESH_COMPONENT_H

#include "math/Vector3.h"
#include "math/Vector2.h"
#include "Component.h"
#include "Mesh.h"

namespace render_e {
class MeshComponent : public Component {
public:
    MeshComponent();
    virtual ~MeshComponent();
    void Render();
    void SetMesh(Mesh *mesh);
    void Release();
private:
    unsigned int vboName;
    unsigned int vboElements;
    int indicesCount;

    int normalOffset;
    int tangentOffset;
    int colorOffset;
    int texture1Offset;
    int texture2Offset;
    int triangleOffset;
#ifdef RENDER_E_NO_VBO  
    unsigned short *indices;
    unsigned char *buffer;
#endif
};
}
#endif	/* MESH_COMPONENT_H */
