/*
Copyright(c) 2022 Gareth Francis

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <vsg/core/Inherit.h>

#include <vsgvr/xr/Common.h>
#include <vsgvr/xr/Instance.h>

namespace vsgvr {
    class Session;

    /**
     * A binding class allowing an XrSpace to be tracked by the Viewer.
     * The specified reference space will be kept updated during rendering (along with any ActionPoseBindings)
     * in order to track elements such as the position of the headset (view) space, or user's local space.
     */
    class VSGVR_DECLSPEC SpaceBinding : public vsg::Inherit<vsg::Object, SpaceBinding>
    {
        public:
            SpaceBinding(vsg::ref_ptr<Instance> instance, XrReferenceSpaceType spaceType);
            virtual ~SpaceBinding();

            XrSpace getSpace() const { return _space; }

            bool getTransformValid() const { return _transformValid; }
            vsg::dmat4 getTransform() const { return _transform; }
            XrReferenceSpaceType getSpaceType() const { return _spaceType; }

            void createSpace(Session* session);
            void destroySpace();

            void setSpaceLocation(XrSpaceLocation location);
        private:
            XrSpace _space = XR_NULL_HANDLE;
            XrReferenceSpaceType _spaceType;

            bool _transformValid = false;
            vsg::dmat4 _transform;
    };
}

EVSG_type_name(vsgvr::SpaceBinding);
