// ============================
// FIREBASE CONFIG
// ============================

import { initializeApp }
from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";

import {
 getFirestore,
 collection,
 addDoc,
 getDocs,
 doc,
 getDoc,
 query,
 where,
 orderBy
}
from "https://www.gstatic.com/firebasejs/10.12.2/firebase-firestore.js";

const firebaseConfig = {
  apiKey: "",
  authDomain: "smartcyclesharing.firebaseapp.com",
  projectId: "smartcyclesharing",
};

const app = initializeApp(firebaseConfig);
const db = getFirestore(app);


// ============================
// ADD USER
// ============================

window.addUser = async function () {

 const name = document.getElementById("name")?.value.trim();
 const phone = document.getElementById("phone")?.value.trim();
 const uid = document.getElementById("uid")?.value
              .trim().toUpperCase();

 if (!name || !phone || !uid) {
  alert("Fill all fields");
  return;
 }

 await addDoc(collection(db,"users"),{
   name,
   phone,
   uid,
   createdAt:new Date()
 });

 alert("✅ User Added");
 loadUsers();
};


// ============================
// LOAD USERS
// ============================

async function loadUsers(){

 const list=document.getElementById("userList");
 if(!list) return;

 list.innerHTML="";

 const snapshot=
 await getDocs(collection(db,"users"));

 snapshot.forEach(docSnap=>{

   const user=docSnap.data();

   const div=document.createElement("div");
   div.className="user-card";

   div.innerHTML=`
     <h3>${user.name}</h3>
     <p>Phone: ${user.phone}</p>
     <p>UID: ${user.uid}</p>

     <button onclick="openUser('${docSnap.id}')">
       View Rental History
     </button>
   `;

   list.appendChild(div);
 });
}


// ============================
// OPEN USER PAGE
// ============================

window.openUser=function(id){
 window.location.href=`user.html?id=${id}`;
};


// ============================
// GET USER ID FROM URL
// ============================

function getUserIdFromURL(){
 const params=new URLSearchParams(window.location.search);
 return params.get("id");
}


// ============================
// LOAD PER USER HISTORY
// ============================

async function loadUserRentals(){

 const rentalList=document.getElementById("rentalList");
 if(!rentalList) return;

 const userId=getUserIdFromURL();
 if(!userId) return;

 rentalList.innerHTML="Loading...";

 try{

   // ---- get user ----
   const userRef=doc(db,"users",userId);
   const userSnap=await getDoc(userRef);

   if(!userSnap.exists()){
     rentalList.innerHTML="User not found";
     return;
   }

   const user=userSnap.data();

   document.getElementById("username")
     .innerText=`Rental History - ${user.name}`;

   // ✅ QUERY HISTORY COLLECTION
   const historyQuery=query(
     collection(db,"history"),
     where("uid","==",user.uid),
     orderBy("startTime","desc")
   );

   const historySnap=
     await getDocs(historyQuery);

   rentalList.innerHTML="";

   if(historySnap.empty){
     rentalList.innerHTML="No rentals found";
     return;
   }

   historySnap.forEach(doc=>{

     const h=doc.data();

     const div=document.createElement("div");
     div.className="rental-card";

     div.innerHTML=`
       <p><b>Start Time:</b> ${h.startTime}</p>
       <p><b>End Time:</b> ${h.endTime}</p>

       <p><b>Start Location:</b>
       ${h.startLocation}</p>

       <p><b>End Location:</b>
       ${h.endLocation}</p>

       <p><b>Duration:</b>
       ${h.duration} minutes</p>

       <p><b>Fee:</b>
       ₹${h.fee}</p>
     `;

     rentalList.appendChild(div);
   });

 }
 catch(err){
   console.error(err);
   rentalList.innerHTML="Error loading history";
 }
}


// ============================
// SAFE PAGE LOAD
// ============================

window.onload=()=>{

 if(document.getElementById("userList"))
   loadUsers();

 if(document.getElementById("rentalList"))
   loadUserRentals();
};
