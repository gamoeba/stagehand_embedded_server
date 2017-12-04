var stagehand = stagehand || {}

stagehand.main = (
    function(){
        var self = {}

        var matmap = {}
        var cameraObject = null
        var first = true

        var testindex = 0
        var testdata = [ 'testdata/test.json', 'testdata/test2.json', 'testdata/test3.json' ]

        function isNumeric(input)
        {
            return (input - 0) == input && (''+input).trim().length > 0;
        }

        function formatNumber(input) {
            if (isNumeric(input)) {
                // slightly clumsy. First parseFloat is to convert to a number format
                // second parseFloat is to remove extraneous 0's from the result of toFixed
                return parseFloat(parseFloat(input).toFixed(4))
            }
            return input;
        }

        function updateTable() {
            $("#maintable").colResizable({
                liveDrag:true, 
                fixed:true,
                postbackSafe:true,
                partialRefresh:true
            })
        }

        function onWindowResize( event ) {
            var w = window.innerWidth
            var h = window.innerHeight

            //$('#widget').trigger('resize')
            //$('#leftsection').trigger('resize')
            $('#widget').width(w-50).height(h).split({orientation:'vertical', position:'75%'});
            $('#leftsection').width('100%').height(h).split({orientation:'vertical', position:'25%'});
            $('#toolbar').height(h)        

            updateTable()
            
            stagehand.renderer.render()

        }

        self.init = function() {
            window.addEventListener( 'resize', onWindowResize, false );
            $('#widget').on('splitter.resize', function(e) {
                stagehand.renderer.render()
                updateTable()
            })
            $('#leftsection').on('splitter.resize', function(e) {
                stagehand.renderer.render()
                updateTable()
            })
            container = document.getElementById( 'scenegraph' );
            stagehand.renderer.initRender(container);
            var item = localStorage.getItem('theme')
            if (item) {
                self.swapStyleSheet(item)
            } else {
                self.swapStyleSheet('css/ocean.css')
            }
            
        }

        self.swapStyleSheet = function(sheet){
            localStorage.setItem('theme', sheet)
            document.getElementById('themecss').setAttribute('href', sheet);
            setTimeout(function(){
                // slight delay to give stylesheet time to load
                // TODO: find better way of achieving this
                stagehand.renderer.updateMaterials()
                stagehand.renderer.render()
                stagehand.intro.update()
            },500)
        }

        self.selectedNode = function(nodeId) {
            var actorlist = stagehand.actorparser.getActorList();
            var obj = actorlist[nodeId];
            stagehand.renderer.setSelection(nodeId)
            stagehand.renderer.updateScene();
            stagehand.renderer.render(); 
            var propHtml = "<h3>Actor: " + nodeId + " "+ obj.Name + "</h3><table id='maintable' style='overflow:auto' >"
            var rownum = 0
            for (var o in obj.properties) {
                var altColor
                var rowclass
                if (rownum%2==1) {
                    altColor = " gridAltDarkColor"
                    rowclass = "darkrow"
                } else {
                    altColor = " gridAltLightColor"
                    rowclass = "lightrow"
                }

                propHtml += "<tr class='"+rowclass+"'>"
                propHtml +="<td class='left'>"
                propHtml += obj.properties[o][0];
                propHtml += "</td>"
                propHtml += "<td class='right'>"
                var propValue = (obj.properties[o][1]).trim();
                if (propValue.startsWith('[')){
                    try {
                        var parsed = JSON.parse(propValue);
                        if ( Array.isArray(parsed)) {
                            if ( Array.isArray(parsed[0])) {
                                propHtml += "<div class='nestedtable'>"
                                for (var j = 0; j < parsed.length; j++) 
                                {
                                    propHtml += "<div>"
                                    var row = parsed[j];

                                     var classtype = ""
                                    switch (row.length) {
                                        case 1:
                                            classtype += "row1"
                                            break;
                                        case 2:
                                            classtype += "row2"
                                            break;
                                        case 3 :
                                            classtype += "row3"
                                            break;
                                        case 4 :
                                            classtype += "row4"
                                            break;
                                        default:

                                    }
                                  
                                    for (var k = 0; k < row.length; k++) 
                                    {
                                        var divclass = classtype
                                        if ((j+k)%2==1) {
                                            divclass += altColor
                                        }
                                        propHtml += "<div class='"+ divclass + "' contenteditable='true' >"
                                        propHtml += formatNumber(row[k])
                                        propHtml += "</div>"
                                    }
                                    propHtml += "</div>"
                                }
                                propHtml += "</div>"

                            } else {
                                propHtml += "<div>"
                                var classtype = ""
                                switch (parsed.length) {
                                    case 1:
                                        classtype += "row1"
                                        break;
                                    case 2:
                                        classtype += "row2"
                                        break;
                                    case 3 :
                                        classtype += "row3"
                                        break;
                                    case 4 :
                                        classtype += "row4"
                                        break;
                                    default:

                                }

                                
                                  
                                for (var j = 0; j < parsed.length; j++) 
                                {
                                    var divclass = classtype
                                    if (j%2==1) {
                                        divclass += altColor
                                    }
                                    propHtml += "<div class='" + divclass + "' contenteditable='true'>"
                                    propHtml += formatNumber(parsed[j])
                                    propHtml += "</div>"
                                }
                                propHtml += "</div>"
                            }
                        } else {
                            propHtml += obj.properties[o][1];
                        }
                    } catch (err) {


                    }
                } else {
                    propHtml += formatNumber(obj.properties[o][1]);

                }
                propHtml += "</td>"
                propHtml += "</tr>"
                rownum ++;
            }
            propHtml += "</table>"

            //$("#maintable").colResizable({disable:true}) 
            $("#proptable").html(propHtml);
            updateTable()
            var fgColor = utils.rgb2hex( $('#container').css('color') )
            $("#proptable").niceScroll({cursorborder: fgColor ,cursorcolor: fgColor ,boxzoom:false})
            $('[contenteditable]').keypress(function(e){ return e.which != 13; });

        }

        self.newScene = function(obj) {
            var list = obj.Name + "\n";
            var len = obj.children.length;
            for (var x = 0; x < len; x++) {
                list += obj.children[x].Name + "\n";
            }
            stagehand.actorparser.parseActors(obj);
            var m = stagehand.actorparser.getActorList();
            var s = stagehand.actorparser.getSpecialList()
            var tl = stagehand.actorparser.getTreeList()
            
            $('#actortree').jstree("destroy") // remove any previous tree, otherwise reinitialisation doesn't work properly
            $('#actortree').html(tl)
            $('#actortree').on("changed.jstree", function (e, data) {
                if(data.selected.length) {
                        // TODO embed the node id as a property to make it independent of the text format
                        var nodeId = (data.instance.get_node(data.selected[0]).text).split(":")[0]
                        self.selectedNode(nodeId);
                }
            })

            $('#actortree').jstree({
                        "core" : {
                            "animation" : 0,
                            "themes" : { "stripes" : true }
            }});
            var fgColor = utils.rgb2hex( $('#container').css('color') )
            //var fgColor = "#ff00ff"
            $('#actortree').niceScroll({cursorborder: fgColor ,cursorcolor: fgColor ,boxzoom:false})

            matmap = {}
            
            for (var key in m) {
                var obj = m[key];
                var wm = stagehand.actorparser.getMatrix4(obj, 'worldMatrix');
                var size = stagehand.actorparser.getVector3(obj, 'size');
                var scale = new THREE.Matrix4().makeScale(size.x, size.y, 0.05)
                wm.transpose();
                wm.multiply(scale);
                matmap[obj.id] = { matrix: wm, scale: scale, visible: obj.IsVisible, overallVisible: obj.overallVisible };
            }

            cameraObject = stagehand.actorparser.getCamera()
            stagehand.renderer.setScene(matmap, cameraObject)
            stagehand.renderer.updateScene()
            stagehand.renderer.render()

        }

        self.updateWS = function() {
            var ws = new WebSocket('ws://' + (location.host ? location.host : "localhost:8080") + "/", "stagehand-protocol");
            ws.onopen = function() {
                document.body.style.backgroundColor = '#000';
                ws.send('dump_scene\n');
            };
            ws.onclose = function() {
                document.body.style.backgroundColor = null;
            };
            ws.onmessage = function(event) {
                var js = event.data;
                ws.close();
                var obj = JSON.parse(js);
                self.newScene(obj)
            	stagehand.intro.hideLogo()
            }
        }


        self.updateDemo = function() {
            stagehand.intro.hideLogo()
            var testurl = testdata[testindex % testdata.length]
            testindex++
            $.getJSON(testurl, function(json) {
                self.newScene(json);
            });
        }
        self.update = function() {
            var url = "demo.txt"
            $.get(url).done(function() { 
                // run the demo if file exists 
                self.updateDemo()
            }).fail(function() {
                // run normally
                self.updateWS()
            })
        }
        return self
    })()

$('document').ready(function(){
    stagehand.main.init()
    stagehand.intro.start()
})

