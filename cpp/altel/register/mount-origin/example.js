
let alive = 0;

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
    let wsa = [];
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
}


document.addEventListener("DOMContentLoaded", DOMContentLoadedListener, false);
