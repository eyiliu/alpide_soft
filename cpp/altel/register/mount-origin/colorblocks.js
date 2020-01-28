
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

let groupSpacingX = 1;
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
    let n_ev = Math.floor(Math.random() * 10);
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

let colorScale = d3.scaleSequential()
    .domain([0, 256])
    .interpolator(d3.interpolatePlasma);

function updateCanvas(canvas) {
    let context = canvas.node().getContext('2d');
    for(d of data){
        if(! d.flushed){
            d.flushed = true;
            context.fillStyle = colorScale(d.hit_count);
            context.fillRect(d.x_position, d.y_position, cellSize, cellSize);
        }
    }
}

// 


function DOMContentLoadedListener_colorblocks() {
    for( let i = 0; i<cellNumber; i++ ){
        let x1 = Math.floor(i % cellNumberX);
        let xg = Math.floor(x1 / groupCellNumberX);
        let y1 = Math.floor(i / cellNumberX);
        let yg = Math.floor(y1 / groupCellNumberY);
        let x = (cellSpacing + cellSize) * x1 + groupSpacingX * (xg+1);
        let y = (cellSpacing + cellSize) * y1 + groupSpacingY * (yg+1);
        data.push( { hit_count: 0, flushed: true, x_position: x, y_position: y} );
    }
    
    let mainCanvas = d3.select('#container0')
        .append('canvas')
        .classed('mainCanvas', true)
        .attr('width', width)
        .attr('height', height);
    
    dummyEventTimer();
    function dummyEventTimer(){
        let ev_array = getEventArray();
        //UpdateData
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
                data[cellN].hit_count += 1;
                data[cellN].flushed = false;
            }
        }
        setTimeout(dummyEventTimer, 100);
    }

    updateCanvasTimer();
    function updateCanvasTimer(){
        updateCanvas(mainCanvas);
        setTimeout(updateCanvasTimer, 1000);
    }
}

document.addEventListener("DOMContentLoaded", DOMContentLoadedListener_colorblocks);
