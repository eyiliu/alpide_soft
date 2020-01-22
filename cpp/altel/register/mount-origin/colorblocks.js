
function DOMContentLoadedListener_colorblocks() {
    var ev_0 = {level: 4, trigger: 0, ext: 1, geometry:[1024, 512, 1], data:{type: "hit_xyz_array", hit_xyz_array:[[100,100,1] [110,100,1]] } };
    var ev_1 = {level: 4, trigger: 1, ext: 1, geometry:[1024, 512, 1], data:{type: "hit_xyz_array", hit_xyz_array:[[200,100,1] [210,100,1]] } };
    var ev_2 = {level: 4, trigger: 2, ext: 1, geometry:[1024, 512, 1], data:{type: "hit_xyz_array", hit_xyz_array:[[200,200,1] [250,100,1]] } };
    
    // settings for a grid with 40 cells in a row and 2x5 cells in a group
    var pixelNumberX = 1024;
    var pixelNumberY = 512;
    var groupPixelNumberX = 32;
    var groupPixelNumberY = 512;
    
    var scalerFactorX = 4;
    var scalerFactorY = 4;
    
    var cellNumberX = Math.ceil(pixelNumberX/scalerFactorX);
    var cellNumberY = Math.ceil(pixelNumberY/scalerFactorY);
    var cellNumber = cellNumberX * cellNumberY;
    
    var groupCellNumberX = Math.ceil(groupPixelNumberX/scalerFactorX);
    var groupCellNumberY = Math.ceil(groupPixelNumberX/scalerFactorX);
    var groupNumberX = Math.ceil(cellNumberX / groupCellNumberX);
    var groupNumberY = Math.ceil(cellNumberY / groupCellNumberY);
    
    var groupSpacingX = 3;
    var groupSpacingY = 1;
    var cellSpacing = 1;
    var cellSize = 2;
    
    var width =  groupSpacingX * (groupNumberX +1 ) + (cellSize + cellSpacing) * cellNumberX;
    var height = groupSpacingY * (groupNumberY +1 ) + (cellSize + cellSpacing) * cellNumberY;    
    
    var data = [];
    for( let i = 0; i<cellNumber; i++ ){
        data.push( { value: i, id:i, x: i/10}   );
    }
    
    var mainCanvas = d3.select('#container')
        .append('canvas')
        .classed('mainCanvas', true)
        .attr('width', width)
        .attr('height', height);

    // === Bind data to custom elements === //
    var customBase = document.createElement('custom');
    var custom = d3.select(customBase); // this is our svg replacement
    databind(data);
    var t = d3.timer(function(elapsed) {draw(mainCanvas);if (elapsed > 300) t.stop();});
    
    // === Bind and draw functions === //
    function databind(data) {
        // var colorScale = d3.scaleSequential(d3.interpolateSpectral).domain(d3.extent(data, function(d) { return d.value; })); 
        var colorScale = d3.scaleSequential()
            .domain([0, cellNumber])
            .interpolator(d3.interpolateRainbow);

        var join = custom.selectAll('custom.rect').data(data);
        var enterSel = join.enter()
	    .append('custom')
	    .attr('class', 'rect')
            .attr('id', function(d){return "cell" + d.id;} )
	    .attr('x', function(d, i) {
                var x1 = Math.floor(i % cellNumberX);
                var xg = Math.floor(x1 / groupCellNumberX);
	        return (cellSpacing + cellSize) * x1 + groupSpacingX * (xg+1);
	    })
	    .attr('y', function(d, i) {
                var y1 = Math.floor(i / cellNumberX);
                var yg = Math.floor(y1 / groupCellNumberY);
	        return (cellSpacing + cellSize) * y1 + groupSpacingY * (yg+1);
	    })
	    .attr('width', 0)
	    .attr('height', 0)
        ;

        join.merge(enterSel)
	    .transition()
	    .attr('width', cellSize)
	    .attr('height', cellSize)
	    .attr('fillStyle', function(d) { return colorScale(d.value); })

        var exitSel = join.exit()
	    .transition()
	    .attr('width', 0)
	    .attr('height', 0)
	    .remove();

    } // databind()

    function draw(canvas) {
        var context = canvas.node().getContext('2d');
        context.clearRect(0, 0, width, height);
        var elements = custom.selectAll('custom.rect') // this is the same as the join variable, but used here to draw
        elements.each(function(d,i) { // for each virtual/custom element...
	    var node = d3.select(this);
	    context.fillStyle =  node.attr('fillStyle'); // <--- new: node colour depends on the canvas we draw 
	    context.fillRect(node.attr('x'), node.attr('y'), node.attr('width'), node.attr('height'))
        });
    }

    var value_n = 0;

    var trigger_n = 0
    function getEventArray(){
        let ev_array = new Array();
        let n_ev = Math.floor(Math.random() * 5);
        for(let i = 0; i< n_ev; i++){
            trigger_n ++;
            let ev = {type:"alpide", level: 4, trigger: trigger_n, ext: 1, geometry:[1024, 512, 6], data:{type: "hit_xyz_array"} };
            let hit_array = new Array();
            let n_hit = Math.floor(Math.random() * 5);
            for(let j = 0; j<n_hit; j++){
                let xy = Math.floor(Math.random() * 128);
                hit_array.push( [ xy, xy, 1 ] );
            }
            ev.hit_xyz_array= hit_array;
            ev_array.push(ev);
        }
        console.log(ev_array);
        return ev_array;
    }

    function dummyEventTimer(){
        let ev_array = getEventArray();
        
        let n_ev =  ev_array.length;
        for(let i=0; i< n_ev; i++ ){
            let ev = ev_array[i];
            let hit_array = ev_array[i].hit_xyz_array;
            let n_hit = hit_array.length;
            for(let j=0; j< n_hit; j++ ){
                let x = hit_array[j][0];
                let y = hit_array[j][1];
                let z = hit_array[j][2];
                console.log(x);
                console.log(y);
                console.log(z);
            }
        }
        setTimeout(dummyEventTimer, 2000);
    }
       
    function button_fun(){
        value_n  = value_n + 10;
        if (value_n > 120 ){
            value_n = 0;
        }
                
        var data = [];
        for( let i = 0; i<cellNumber; i++ ){
            data.push( { value: value_n, id:i, x: i/10}   );
        }
        
        databind(data);
        console.log("callling");
        var t = d3.timer(function(elapsed) {draw(mainCanvas);if (elapsed > 300) t.stop();});
        console.log("called");
        console.log(value_n);
        setTimeout(dummyEventTimer, 2000);
    }
    
    document.getElementById("updatebutton").addEventListener("click", button_fun);
}


document.addEventListener("DOMContentLoaded", DOMContentLoadedListener_colorblocks, false);
