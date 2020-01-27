
// matrix size
let pixelNumberX = 1024;
let pixelNumberY = 512;
let groupPixelNumberX = 32;
let groupPixelNumberY = 512;

let scalerFactorX = 4;
let scalerFactorY = 4;    

let cellNumberX = Math.ceil(pixelNumberX/scalerFactorX);
let cellNumberY = Math.ceil(pixelNumberY/scalerFactorY);
let cellNumber = cellNumberX * cellNumberY;
    
let groupCellNumberX = Math.ceil(groupPixelNumberX/scalerFactorX);
let groupCellNumberY = Math.ceil(groupPixelNumberY/scalerFactorY);
let groupNumberX = Math.ceil(cellNumberX / groupCellNumberX);
let groupNumberY = Math.ceil(cellNumberY / groupCellNumberY);

let groupSpacingX = 3;
let groupSpacingY = 1;
let cellSpacing = 1;
let cellSize = 2;

let width =  groupSpacingX * (groupNumberX +1 ) + (cellSize + cellSpacing) * cellNumberX;
let height = groupSpacingY * (groupNumberY +1 ) + (cellSize + cellSpacing) * cellNumberY;    
//

let data = [];

//
let trigger_n = 0
function getEventArray(){
    let ev_array = new Array();
    let n_ev = Math.floor(Math.random() * 5);
    for(let i = 0; i< n_ev; i++){
        trigger_n ++;
        let ev = {detector_type:"alpide", trigger: trigger_n, ext: 1, geometry:[1024, 512, 6], data_type: "hit_xyz_array"};
        let hit_array = new Array();
        let n_hit = Math.floor(Math.random() * 10);
        for(let j = 0; j<n_hit; j++){
            let pixelX = Math.floor(Math.random() * pixelNumberX);
            let pixelY = Math.floor(Math.random() * pixelNumberY);
            hit_array.push( [ pixelX, pixelY, 1 ] );
        }
        ev.hit_xyz_array= hit_array;
        ev_array.push(ev);
    }
    console.log(ev_array);
    return ev_array;
}
//

// global width, height
function draw(canvas, custom) {
    let context = canvas.node().getContext('2d');
    context.clearRect(0, 0, width, height);
    let elements = custom.selectAll('custom.rect');
    elements.each(function(d,i) {
	let node = d3.select(this);
	context.fillStyle =  node.attr('fillStyle');
	context.fillRect(node.attr('x'), node.attr('y'), node.attr('width'), node.attr('height'))
    });
}


// === Bind and draw functions === //
function databind(data, custom) {    
    let colorScale = d3.scaleSequential()
        .domain([0, 256])
        .interpolator(d3.interpolatePlasma);

    let join = custom.selectAll('custom.rect').data(data);
    let enterSel = join.enter()
	.append('custom')
	.attr('class', 'rect')
        .attr('id', function(d){return "cell" + d.id;} )
	.attr('x', function(d, i) {
            let x1 = Math.floor(i % cellNumberX);
            let xg = Math.floor(x1 / groupCellNumberX);
            return (cellSpacing + cellSize) * x1 + groupSpacingX * (xg+1);
        })
	.attr('y', function(d, i) {
            let y1 = Math.floor(i / cellNumberX);
            let yg = Math.floor(y1 / groupCellNumberY);
            return (cellSpacing + cellSize) * y1 + groupSpacingY * (yg+1);
        })
        .attr('width', 0)
        .attr('height', 0)
    ;

    join.merge(enterSel)
        .transition()
        .attr('width', cellSize)
        .attr('height', cellSize)
        .attr('fillStyle', function(d) { return colorScale(d.hit_count); })

    let exitSel = join.exit()
        .transition()
        .attr('width', 0)
        .attr('height', 0)
        .remove();
}


function DOMContentLoadedListener_colorblocks() {
    for( let i = 0; i<cellNumber; i++ ){
        data.push( { hit_count: 0, id:i}   );
    }
    
    let customBase = document.createElement('custom');
    let custom = d3.select(customBase); // this is our svg replacement

    let mainCanvas = d3.select('#container')
    .append('canvas')
    .classed('mainCanvas', true)
    .attr('width', width)
    .attr('height', height);
    
    dummyEventTimer();
    function dummyEventTimer(){
        let ev_array = getEventArray();
        let n_ev =  ev_array.length;
        for(let i=0; i< n_ev; i++ ){
            let ev = ev_array[i];
            let hit_array = ev_array[i].hit_xyz_array;
            let n_hit = hit_array.length;
            for(let j=0; j< n_hit; j++ ){
                let pixelX = hit_array[j][0];
                let pixelY = hit_array[j][1];
                let pixelZ = hit_array[j][2];
                cellX=Math.floor(pixelX/scalerFactorX);
                cellY=Math.floor(pixelY/scalerFactorY);
                cellN= cellX + cellNumberX * cellY;
                data[cellN].hit_count = 255;
            }
        }
        databind(data, custom);
        // console.log("callling");
        draw(mainCanvas, custom);
        // console.log("called");
        setTimeout(dummyEventTimer, 1000);
    }
}

document.addEventListener("DOMContentLoaded", DOMContentLoadedListener_colorblocks);
