function openTAB(btnName, tabName) {
    // Declare all variables
    var i, tabcontent, tablinks;

    // Get all elements with class="tabcontent" and hide them
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Get all elements with class="tablinks" and remove the class "active"
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }
    
    // Show the current tab, and add an "active" class to the button that opened the tab
    document.getElementById(tabName).style.display = "block";
    document.getElementById(btnName).className += " active";
} 

function DOMContentLoadedListener() {
    document.getElementById("btn_aux0").addEventListener("click", function(){openTAB("btn_aux0","tab_aux0")});
    document.getElementById("btn_alpide0").addEventListener("click", function(){openTAB("btn_alpide0","tab_alpide0")});
    document.getElementById("btn_alpide1").addEventListener("click", function(){openTAB("btn_alpide1","tab_alpide1")});
    document.getElementById("btn_alpide2").addEventListener("click", function(){openTAB("btn_alpide2","tab_alpide2")});
    document.getElementById("btn_alpide3").addEventListener("click", function(){openTAB("btn_alpide3","tab_alpide3")});
    document.getElementById("btn_alpide4").addEventListener("click", function(){openTAB("btn_alpide4","tab_alpide4")});
    document.getElementById("btn_alpide5").addEventListener("click", function(){openTAB("btn_alpide5","tab_alpide5")});

    openTAB("btn_aux0","tab_aux0");
}

    
document.addEventListener("DOMContentLoaded", DOMContentLoadedListener, false);
//https://www.w3schools.com/howto/howto_js_tabs.asp
