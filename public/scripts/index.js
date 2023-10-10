// import { auth } from "firebase/auth";
import { getAuth } from 'https://www.gstatic.com/firebasejs/9.18.0/firebase-auth.js'
console.log(auth);

const loginElement = document.querySelector('#login-form');
const contentElement = document.querySelector("#content-sign-in");
const userDetailsElement = document.querySelector('#user-details');
const authBarElement = document.querySelector("#authentication-bar");
var clickShield = false;

// Elements for GPIO states
const stateElement1 = document.getElementById("state1");

// Button Elements
const btn1On = document.getElementById('garage-btn-on');
const btn1Off = document.getElementById('garage-btn-off');


// Database path for GPIO states
var dbPathOutput1 = 'board1/outputs/digital/5';
var dbPathInput1= 'board1/inputs/digital/4';


// Database references
var dbRefOutput1 = firebase.database().ref().child(dbPathOutput1);
var dbPathInput1 = firebase.database().ref().child(dbPathInput1);


// MANAGE LOGIN/LOGOUT UI
export const setupUI = (user) => {
  if (user) {
    //toggle UI elements
    loginElement.style.display = 'none';
    contentElement.style.display = 'block';
    authBarElement.style.display ='block';
    userDetailsElement.style.display ='block';
    userDetailsElement.innerHTML = user.email;

    //Read the input of the reed swith to determine current door state
    dbPathInput1.on('value', snap =>{
        if(snap == 0){ //door closed
            stateElement1.innerText="Closed";
            btn1Off.classList.remove("button-on");
            btn1Off.firstElementChild.classList.remove("fa-gradient");
            btn1Off.classList.add("button-off");
            clickShield = false;
        }
        else if(snap == 1){ //door open
            stateElement1.innerText="Open";
            btn1Off.classList.remove("button-off");
            btn1Off.classList.add("button-on");
            btn1Off.firstElementChild.classList.add("fa-gradient");
            clickShield = false;
        }else{ //Error
            stateElement1.innerText="Error: Reed switch - Invalid state ";
        }
    })

    //Update states depending on the database value
    dbRefOutput1.on('value', snap => {
        // if(snap.val()==1) { //may switch this to be closed as the reed switch with be in closed pos. High value or (1)
        //     stateElement1.innerText="Open";
        //     btn1Off.classList.remove("button-off");
        //     btn1Off.classList.add("button-on");
        //     btn1Off.firstElementChild.classList.add("fa-gradient");
        // }
        // else if(snap.val() == 0){
        //     stateElement1.innerText="Closed";
        //     btn1Off.classList.remove("button-on");
        //     btn1Off.firstElementChild.classList.remove("fa-gradient");
        //     btn1Off.classList.add("button-off");
        // }
        //else if(snap.val() == 2){
        if(snap.val() == 2){
            stateElement1.innerText="Opening";
            // setTimeout(function() {
            //     clickShield = false;
            //     dbRefOutput1.set(1);
            //   }, 2000);
        }
        else if(snap.val() == 3){
            stateElement1.innerText="Closing";
            // setTimeout(function() {
            //     clickShield = false;
            //     dbRefOutput1.set(0);
            // }, 2000);
        }
    });

    // Update database uppon button click
    btn1Off.onclick = () =>{
        if(btn1Off.classList.contains("button-off") && !clickShield){
            clickShield = true;
            console.log('opening...');
            dbRefOutput1.set(2);
        }
        else if(btn1Off.classList.contains("button-on") && !clickShield){
            clickShield = true;
            console.log('closing...');
            dbRefOutput1.set(3);
        }
    }


  // if user is logged out
  } else{
    // toggle UI elements
    loginElement.style.display = 'block';
    authBarElement.style.display ='none';
    userDetailsElement.style.display ='none';
    contentElement.style.display = 'none';
  }



}