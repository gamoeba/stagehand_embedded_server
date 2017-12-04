var stagehand = stagehand || {}

stagehand.intro = (
    function(){
        var self = {}

        var renderer, scene, camera, stats;

        var mesh, uniforms;

        var initTime

        var container
        var hidden = false
        var animating = false
        var backgroundColor = 0x0
        var foregroundColor = 0x0

        var font = null

        function rgb2hex(rgb) {
           rgb = rgb.match(/^rgba?[\s+]?\([\s+]?(\d+)[\s+]?,[\s+]?(\d+)[\s+]?,[\s+]?(\d+)[\s+]?/i);
           return (rgb && rgb.length === 4) ? "#" +
           ("0" + parseInt(rgb[1],10).toString(16)).slice(-2) +
           ("0" + parseInt(rgb[2],10).toString(16)).slice(-2) +
           ("0" + parseInt(rgb[3],10).toString(16)).slice(-2) : '';
        }

        self.start = function() {
            if (!hidden && !animating) {
                animating = true
                var loader = new THREE.FontLoader();
                loader.load( 'js/fonts/helvetiker_bold.typeface.js', function ( f ) {
                    font = f
                    init();
                    initTime = Date.now()
                    animate();

                } );
            }
        }

        self.update = function() {
            if (!animating) {
                backgroundColor = rgb2hex( $('#container').css('background-color') )
                foregroundColor = rgb2hex( $('#container').css('color') )
                initTime = Date.now()
                animate();
            }
        }

        function init() {

            container = document.getElementById('intro')

            backgroundColor = rgb2hex( $('#container').css('background-color') )
            foregroundColor = rgb2hex( $('#container').css('color') )

            camera = new THREE.PerspectiveCamera( 40, container.clientWidth / container.clientHeight, 1, 10000 );
            camera.position.set( 0, 0, 600 );

            scene = new THREE.Scene();

            var geometry = new THREE.TextGeometry( "Stagehand", {

                font: font,

                size: 40,
                height: 5,
                curveSegments: 3,

                bevelThickness: 2,
                bevelSize: 1,
                bevelEnabled: true

            });

            geometry.center();

            var tessellateModifier = new THREE.TessellateModifier( 8 );

            for ( var i = 0; i < 6; i ++ ) {

                tessellateModifier.modify( geometry );

            }

            var explodeModifier = new THREE.ExplodeModifier();
            explodeModifier.modify( geometry );

            var numFaces = geometry.faces.length;

            geometry = new THREE.BufferGeometry().fromGeometry( geometry );

            var colors = new Float32Array( numFaces * 3 * 3 );
            var displacement = new Float32Array( numFaces * 3 * 3 );

            var color = new THREE.Color();
            var offset = new THREE.Color(foregroundColor)

            for ( var f = 0; f < numFaces; f ++ ) {

                var index = 9 * f;
                var factor = 0.2;
                var rndr = Math.random() * factor;// - factor/2
                var rndg = Math.random() * factor;// - factor/2
                var rndb = Math.random() * factor;// - factor/2
 
                var r = (offset.r) - (rndr)
                var g = (offset.g) - (rndr)
                var b = (offset.b) - (rndr)

                color.setRGB( r, g, b );

                var d = 10 * ( 0.5 - Math.random() );

                for ( var i = 0; i < 3; i ++ ) {

                    colors[ index + ( 3 * i )     ] = color.r;
                    colors[ index + ( 3 * i ) + 1 ] = color.g;
                    colors[ index + ( 3 * i ) + 2 ] = color.b;

                    displacement[ index + ( 3 * i )     ] = d;
                    displacement[ index + ( 3 * i ) + 1 ] = d;
                    displacement[ index + ( 3 * i ) + 2 ] = d;

                }

            }

            geometry.addAttribute( 'customColor', new THREE.BufferAttribute( colors, 3 ) );
            geometry.addAttribute( 'displacement', new THREE.BufferAttribute( displacement, 3 ) );

            uniforms = {

                amplitude: { type: "f", value: 0.0 },
                opacity: { type: "f", value: 0.0 },
                "shadeColor" : { type: "c", value: new THREE.Color(foregroundColor) },

            };

            var shaderMaterial = new THREE.ShaderMaterial( {

                uniforms:       uniforms,
                vertexShader:   document.getElementById( 'introvertexshader' ).textContent,
                fragmentShader: document.getElementById( 'introfragmentshader' ).textContent,
                transparent: true

            });

            mesh = new THREE.Mesh( geometry, shaderMaterial );

            scene.add( mesh );


            renderer = new THREE.WebGLRenderer( { antialias: true, alpha : true } );
            renderer.setClearColor(  backgroundColor, 1 );
            renderer.setPixelRatio( window.devicePixelRatio );
            renderer.setSize( container.clientWidth, container.clientHeight );

            
            container.appendChild( renderer.domElement );

            window.addEventListener( 'resize', onWindowResize, false );

        }

        function onWindowResize() {

            var container = document.getElementById('intro')
            camera.aspect = container.clientWidth / container.clientHeight;
            camera.updateProjectionMatrix();

            renderer.setSize( container.clientWidth, container.clientHeight );
            render()

        }


        function animate() {
            var cont = true
            var time = Date.now();
            time -= initTime

            if (time > 1000) {
                time = 1000
                cont = false
            }
            var val = 1 - (time*0.001) 
            uniforms.amplitude.value = 50 * val
            uniforms.opacity.value = 1 - val
            uniforms.shadeColor.value = new THREE.Color(foregroundColor)
            renderer.setClearColor( backgroundColor, 1-val );

            if (cont) {
                requestAnimationFrame( animate );
            } else {
                animating = false
            }

            render();


        }
        self.hideLogo = function() {
            if (!hidden) {
                // only perform first time
                initTime = Date.now()
                animateHide()
            }
        }

        function animateHide() {
            var cont = true
            var time = Date.now();
            time -= initTime

            if (time > 1000) {
                time = 1000
                cont = false
            }
            var val = time * 0.001
            uniforms.amplitude.value = 50 * val
            uniforms.opacity.value = 1 - val
            uniforms.shadeColor.value = new THREE.Color(foregroundColor)
            renderer.setClearColor( backgroundColor, 1-val );

            if (cont) {
                requestAnimationFrame( animateHide );
            } else {
                container.style.visibility = 'hidden'
                hidden = true
            }

            render();


        }

        function render() {
            renderer.render( scene, camera );

        }

        return self
    })()


