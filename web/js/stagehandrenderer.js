var stagehand = stagehand || {}

stagehand.renderer = (
    function(){
        var self = {}

        var camera = null
        var scene = null
        var renderer = null
        var mesh = null
        var material = null
        var selectionMaterial = null
  
        var currentScale = 1.0
        var nextScale = 1.0
        var scaleStep = 0

        var offsetX = 0
        var offsetY = 0
        var diffX = 0
        var diffY = 0
        var translation = new THREE.Matrix4().identity()
      
        var geometry = null
        var linegeometry = null
        var lineMaterial = null
        var selectedLineMaterial = null
    
        var selectionlist = []

        var matmap = {}
        var cameraObject = {} 

        var selected = 0;
        var selIndex = 0
        var selArray = {}

        var mouseDown = false;
        var mouseDownPos = new THREE.Vector3()

        function addListener(element, type, callback, capture) {
            if (element.addEventListener) {
                element.addEventListener(type, callback, capture);
            } else {
                element.attachEvent("on" + type, callback);
            }
        }

        function handleMouseWheel(e)
        {
            var scrollFactor = 1.025
            var e = window.event || e;
            var delta = Math.max(-1, Math.min(1, (e.wheelDelta || -e.detail)));

            if (delta != 0 ) 
            {
                if (delta<0) {
                    currentScale /= scrollFactor
                } else {
                    currentScale *= scrollFactor
                }

                self.updateCamera()
                self.render()
            }
        
        }


        function draggable(element) {
            var dragging = null;

            addListener(element, "mousewheel", handleMouseWheel, false)
            addListener(element, "DOMMouseScroll", handleMouseWheel, false)

            addListener(element, "mousedown", function(e) {
                var e = window.event || e;
                dragging = {
                    clientX: e.clientX,
                    clientY: e.clientY,
                    offsetX: e.offsetX,
                    offsetY: e.offsetY
                };
                if (element.setCapture) element.setCapture();
                $(document).on("selectstart", false);
            });

            addListener(element, "losecapture", function() {
                dragging = null;
            });

            addListener(document, "mouseup", function() {
                
                $(document).off("selectstart", false);
                if (Math.abs(diffX) > 0  || Math.abs(diffY) > 0)
                {
                    offsetX += diffX
                    offsetY += diffY
                    diffX = 0
                    diffY = 0
                } else {
                    if (dragging) {
                        select(dragging)
                    }
                }
                dragging = null;

            }, true);

            var dragTarget = element.setCapture ? element : document;

            addListener(dragTarget, "mousemove", function(e) {
                if (!dragging) return;

                var e = window.event || e;
                diffX = (e.clientX - dragging.clientX);
                diffY = -(e.clientY - dragging.clientY);
          
                self.updateCamera()
                self.render()

            }, true);
        };


        function select(dragging)
        {
            var w = renderer.domElement.clientWidth
            var h = renderer.domElement.clientHeight

            var mouse = new THREE.Vector3() 
            mouse.x = (dragging.offsetX/w*2.0) - 1.0
            mouse.y = -(dragging.offsetY/h*2.0) + 1.0
            mouse.z = 1.0 
            var camerapos = new THREE.Vector3()
            camerapos.setFromMatrixPosition(camera.matrix)

            mouse.unproject( camera )
            raycaster.set( camerapos, mouse.sub( camerapos ).normalize() );

            var intersects = raycaster.intersectObjects( selectionlist );
            var ids = intersects.map(function (e) { 
                return parseInt(e.object.stagehand_id)
            } )

            if (ids.length > 0) 
            {
                if (utils.arraysIdentical(ids,selArray))
                {
                    selIndex++;
                    if (selIndex >= selArray.length) {
                        selIndex = 0
                    }
                } else {
                    selArray = ids
                    selIndex = 0
                }
                var sel = selArray[selIndex]
                stagehand.main.selectedNode(sel)
            }
        }

        self.initRender = function(domElement) {
            renderer = new THREE.WebGLRenderer( { alpha : true } );
            renderer.setClearColor( 0x0, 0.0 );
            renderer.setPixelRatio( window.devicePixelRatio );

            camera = new THREE.Camera();
            domElement.appendChild( renderer.domElement );
            draggable(domElement)
            raycaster = new THREE.Raycaster()
            scene = new THREE.Scene();          

            rectGeom = new THREE.Geometry();
            rectGeom.vertices.push(new THREE.Vector3(-0.5, -0.5, 1));
            rectGeom.vertices.push(new THREE.Vector3(0.5, -0.5, 1));
            rectGeom.vertices.push(new THREE.Vector3(0.5, 0.5, 1));
            rectGeom.vertices.push(new THREE.Vector3(-0.5, 0.5, 1));
            rectGeom.vertices.push(new THREE.Vector3(-0.5, -0.5, 1));

            geometry = new THREE.BoxGeometry( 1.0, 1.0, 1.0 );

               var element = document.getElementById('container')
            var col = $('#container').css('color')
            col = utils.rgb2hex(col)

            lineMaterial = new THREE.LineBasicMaterial( {color: 0x000000, side: THREE.DoubleSide }); 
            selectedLineMaterial = new THREE.LineBasicMaterial( {color: 0xff0000 }); 

            material = new THREE.RawShaderMaterial( {
                uniforms: {
                    "shadeColor" : { type: "c", value: new THREE.Color(col) },
                    "alpha" : { type: "f", value: 0.2 }
            },
            vertexShader: document.getElementById( 'vertexShader' ).textContent,
            fragmentShader: document.getElementById( 'fragmentShader' ).textContent,
            side: THREE.DoubleSide,
            transparent: true

            } );

            selectionMaterial = new THREE.RawShaderMaterial( {

                uniforms: {
                    "shadeColor" : { type: "c", value: new THREE.Color(0xff0000) },
                    "alpha" : { type: "f", value: 0.5 }
                },
            vertexShader: document.getElementById( 'vertexShader' ).textContent,
            fragmentShader: document.getElementById( 'fragmentShader' ).textContent,
            side: THREE.DoubleSide,
            transparent: true

        } );
        }

        self.updateMaterials = function() {
           var element = document.getElementById('container')
            var col = $('#container').css('color')
            col = utils.rgb2hex(col)
            material.uniforms.shadeColor = { type: "c", value: new THREE.Color(col) }
            material.needsUpdate = true
      }

    self.setSelection = function(sel) {
        selected = sel
    }

    self.setScene = function(mat, c) {
        matmap = mat
        cameraObject = c
    }

    self.updateScene = function() {
        var numElements = Object.keys(matmap).length;
        var mat;

        self.updateCamera()
        selectionlist = []

        var len = scene.children.length
        for (var i=scene.children.length-1;i>0;i--) {
            scene.remove(scene.children[i])
        }

        for ( var key in matmap ) {
            var node = matmap[key];
            mat = node.matrix;
            if ((node.overallVisible) && !(mat.elements[0] == 0 && mat.elements[5]==0))
            {
                var object
                var showSelection = (selected == key)
                if (showSelection) {
                    object = new THREE.Mesh( geometry, selectionMaterial );
                } else {
                    object = new THREE.Mesh( geometry, material );
                }

                object.matrixAutoUpdate = false
                object.matrix = mat 
                object.stagehand_id = key
                scene.add( object );
                selectionlist.push(object)

                var lineObj = null
                if (showSelection) 
                    lineObj = new THREE.Line(rectGeom, selectedLineMaterial)
                else
                    lineObj = new THREE.Line(rectGeom, lineMaterial)
                lineObj.matrixAutoUpdate = false
                lineObj.matrix = mat
                scene.add(lineObj)
            } 
        }

        camera.updateMatrixWorld();
    
        var w = $("#scenegraph").width();
        var h = window.innerHeight;
        renderer.setSize( w, h );
    }

    self.updateCamera = function() {
        var w = renderer.domElement.clientWidth
        var h = renderer.domElement.clientHeight
        
        var aspectRatio = stagehand.actorparser.getFloatProperty(cameraObject, 'aspectRatio');
        var pm = stagehand.actorparser.getMatrix4(cameraObject, 'projectionMatrix').transpose();
        var vm = stagehand.actorparser.getMatrix4(cameraObject, 'viewMatrix').transpose();
        var wm = stagehand.actorparser.getMatrix4(cameraObject, 'worldMatrix').transpose();
 
        var postscale = new THREE.Matrix4().makeScale( aspectRatio * currentScale, currentScale, 1.0)
 
        translation.makeTranslation((offsetX+diffX)*2/w, (offsetY+diffY)*2/h, 0)
        pm.multiplyMatrices(translation, pm)
        pm.multiply(vm)
        pm.multiply(postscale)

        camera.projectionMatrix = pm;
        camera.matrix = wm
        camera.matrixAutoUpdate = false
    }

    self.zoomIn = function() {
        nextScale = currentScale * 2
        scaleStep = (nextScale - currentScale)/25
        animate()
    }

     self.zoomOut = function() {
        nextScale = currentScale / 2
        scaleStep = (nextScale - currentScale)/25
        animate()
  
    }

    function animate() {
        currentScale += scaleStep
        if ((scaleStep > 0 && currentScale < nextScale) ||
            (scaleStep < 0 && currentScale > nextScale)) {
            requestAnimationFrame(animate)
        } else {
            currentScale = nextScale
        }
        self.updateCamera()
        self.render()
    }


    self.render = function() 
    {
        var w = $("#scenegraph").width();
        var h = window.innerHeight;
        renderer.setSize( w, h );
        renderer.render( scene, camera );
    }

    return self
    })()

