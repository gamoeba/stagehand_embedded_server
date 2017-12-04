
var stagehand = stagehand || {}

stagehand.actorparser = (
    function(){
        var actormap = {}
        var specialmap = {}
        var treelist = ""
        var camera = null;
        var self = {}
        var parseActors = function(obj, overallVisible) {
            treelist += "<ul><li data-jstree='{ \"opened\" : true, \"icon\" : \"\" }' >" +obj.id+": "+ obj.Name
            if (obj.Name == "DefaultCamera")
            {
                camera = obj
                specialmap["DefaultCamera"] = obj
            }
            if (overallVisible && (parseInt(obj.IsVisible)==0)) {
                overallVisible = false
            }
            obj.overallVisible = overallVisible
            actormap[obj.id] = obj
            var len = obj.children.length
            for (var x = 0; x < len; x++) {
                parseActors(obj.children[x], overallVisible)
            }
            treelist += "</il>"
            treelist += "</ul>"
        }

        self.parseActors = function(obj) {
            //clear maps before calling internal function
            treelist = ""
            actormap = {}
            specialmap = {}
            parseActors(obj, true)
        }

        self.getActorList = function() {
            return actormap
        }

        self.getSpecialList = function() {
            return specialmap
        }

        self.getCamera = function() {
            return camera;
        }

        self.getTreeList = function() {
            return treelist;
        }

        var getProperty = function(obj, propName) {
            var index = obj.properties.map(function(e) { return e[0]; }).indexOf(propName);
            return obj.properties[index][1];
        }

        self.getFloatProperty = function(obj, propName) {
            return parseFloat(getProperty(obj, propName));

        }

        self.getVector3 = function(obj, propName) {
            var v = getProperty(obj, propName);
            var vec = JSON.parse(v);
            return new THREE.Vector3(vec[0],vec[1],vec[2]);
        }

        self.getMatrix4 = function(obj, propName) {
            var w = getProperty(obj, propName);
            var wm = JSON.parse(w);
            var m = new THREE.Matrix4();
            m.set(wm[0][0], wm[0][1], wm[0][2], wm[0][3],
                wm[1][0], wm[1][1], wm[1][2], wm[1][3],
                wm[2][0], wm[2][1], wm[2][2], wm[2][3],
                wm[3][0], wm[3][1], wm[3][2], wm[3][3]);

            return m;
        }


        return self
    })()
