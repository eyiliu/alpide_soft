
let alive = 0;
let wsa = [];


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

       
    setTimeout(dummyEventTimer, 2000);
}


function get_appropriate_ws_url(extra_url){
    let pcol;
    let u = document.URL;
    if (u.substring(0, 5) === "https") {
	pcol = "wss://";
	u = u.substr(8);
    }
    else {
	pcol = "ws://";
	if (u.substring(0, 4) === "http")
	    u = u.substr(7);
    }
    u = u.split("/");
    return pcol + u[0] + "/" + extra_url;
}

function new_ws(urlpath, protocol){
    if (typeof MozWebSocket != "undefined")
	return new MozWebSocket(urlpath, protocol);
    return new WebSocket(urlpath, protocol);
}

function on_ws_open() {
    document.getElementById("r").disabled = 0;
    alive++;
};

function on_ws_close(){
    alive --;
    if (alive === 0)
	document.getElementById("r").disabled = 1;
};

function DOMContentLoadedListener() {
    ws = new_ws(get_appropriate_ws_url(""), "lws-minimal");
    wsa.push(ws);
    ws.onopen = on_ws_open;
    ws.onclose = on_ws_close;
        
    let head = 0;
    let tail = 0;
    let ring = [];
    ws.onmessage = function got_packet(msg) {
	let n;
        let s = "";
	
	ring[head] = msg.data + "\n";
	head = (head + 1) % 50;
	if (tail === head)
	    tail = (tail + 1) % 50;
	
	n = tail;
	do {
	    s = s + ring[n];
	    n = (n + 1) % 50;
	} while (n !== head);
	
	document.getElementById("r").value = s; 
	document.getElementById("r").scrollTop =
	    document.getElementById("r").scrollHeight;
    };
    
    function sendmsg(){
	ws.send(document.getElementById("m").value);
	document.getElementById("m").value = "";
    }
    
    document.getElementById("b").addEventListener("click", sendmsg);   

    dummyEventTimer();
    
    document.getElementById("btn_download").addEventListener("click", startDownload_start);
    //document.getElementById("btn_download_stop").addEventListener("click", startDownload_stop);
    //TODO: start and stop download
}


function startDownload_start(){
    streamSaver.mitm = location.href.substr(0, location.href.lastIndexOf('/')) +'/mitm.html'
    let fileStream = streamSaver.createWriteStream('sample_yi.txt')
    let writer = fileStream.getWriter()
    let a = new Uint8Array(1024).fill(97)
    writer.write(a);
    writer.close();
}


document.addEventListener("DOMContentLoaded", DOMContentLoadedListener, false);
