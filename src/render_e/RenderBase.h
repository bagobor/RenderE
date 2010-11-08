/*
 *  render_base.h
 *  RenderE
 */
#ifndef RENDER_E_RENDERBASE_H
#define RENDER_E_RENDERBASE_H

#include <vector>
#include "SceneObject.h"

using std::vector;

namespace render_e {

// forward declaration
class SceneObject;

class RenderBase {
private:
    static RenderBase *s_instance;
    vector<SceneObject*> sceneObjects;
    vector<SceneObject*> cameras;
public:
    void Display();

    void AddSceneObject(SceneObject *sceneObject);
    void DeleteSceneObject(SceneObject *sceneObject);

    /**
     * Singleton pattern
     * @return the render base instance
     */
    static RenderBase* instance() {
        if (!s_instance) {
            s_instance = new RenderBase();
        }
        return s_instance;
    }
};

}
#endif
