var utils = utils || {}

utils = {
    rgb2hex : function(rgb) {
       rgb = rgb.match(/^rgba?[\s+]?\([\s+]?(\d+)[\s+]?,[\s+]?(\d+)[\s+]?,[\s+]?(\d+)[\s+]?/i);
       return (rgb && rgb.length === 4) ? "#" +
       ("0" + parseInt(rgb[1],10).toString(16)).slice(-2) +
       ("0" + parseInt(rgb[2],10).toString(16)).slice(-2) +
       ("0" + parseInt(rgb[3],10).toString(16)).slice(-2) : '';
    },
    arraysIdentical : function(arr1, arr2) {
        if (arr1.length != arr2.length) return false

        for (var i = 0 ; i < arr1.length; i++ ) {
            if (arr1[i] != arr2[i] ) {
                return false
            }
        }
        return true
    } 

}


