
  // Your web app's Firebase configuration
  const firebaseConfig = {
    
    apiKey: "AIzaSyBN4NyyNUkh6lqnlKx0O5e58NdzCMWhSsQ",
  authDomain: "rifid-ee24a.firebaseapp.com",
  projectId: "rifid-ee24a",
  storageBucket: "rifid-ee24a.appspot.com",
  messagingSenderId: "1085883216061",
  appId: "1:1085883216061:web:fa42175de3878f2b8e07d4",
  };

  // Initialize Firebase
  firebase.initializeApp(firebaseConfig);
  
  //======================= Door ===============================
  var digital_door = document.getElementById("door-click");
    digital_door.onclick = function() {
        if (document.getElementById("digital-door-checkbox").checked) {
            firebase.database().ref("/Door").update({
                "State": "OFF"
            })
        } else {
            firebase.database().ref("/Door").update({
                "State": "ON"
            })
        }

        firebase.database().ref("/Door/State").once("value", function(snapshot) {
            var doorStatus = snapshot.val();

            let displayImage = document.getElementById('house-lock');
            if (doorStatus === "ON") {
                displayImage.src = '/img/DOOR_OPEN.png';
            } else {
                displayImage.src = '/img/DOOR_CLOSE.png';
            }
        });
    }

//=======================toggleButton - amination ===============================
var toggleButton = document.getElementById('toggleButton');
var currentState = "OFF";

toggleButton.addEventListener('click', function() {
  if (toggleButton.classList.contains('clicked')) {
    toggleButton.classList.remove('clicked');
  } else {
    toggleButton.classList.add('clicked');
  }
});

//=======================toggleButton ===============================
toggleButton.onclick = function() {
    if (currentState === "OFF") {
      currentState = "ON";
      console.log("ON");
    } else {
      currentState = "OFF";
      console.log("OFF");
    }
    
    firebase.database().ref("/ledState").update({
      "State": currentState
    });
  };
//=======================CardValid ===============================
var firebaseRef1 = firebase.database().ref("Card_Valid");
var outputContainer1 = document.querySelector('#CardValid');

firebaseRef1.on("value", function(snapshot) {
    var output1 = '';
  
    snapshot.forEach(function(childSnapshot) {
      var childKey1 = childSnapshot.key;
      var childValue1 = childSnapshot.val();
  
      output1 += "<div>" + childValue1 + " <button class='deleteButton' data-key='" + childKey1 + "'><i class='fa fa-trash'></i></button></div>";
    });
  
    // Xóa nội dung HTML cũ
    outputContainer1.innerHTML = '';
  
    // Thêm nội dung HTML mới
    outputContainer1.innerHTML = output1;
  
    // Gắn sự kiện click cho nút xóa
    // Gắn sự kiện click cho nút xóa
    var deleteButtons = document.getElementsByClassName('deleteButton');
    for (var i = 0; i < deleteButtons.length; i++) {
      deleteButtons[i].addEventListener('click', function() {
        var key = this.getAttribute('data-key');
        // Display a confirmation dialog
        var confirmation = confirm("Are you sure you want to delete this card?");
      
        // If the user confirms the deletion, remove the card from Firebase
        if (confirmation) {
          deleteCard(key);
        }
      });
    }
  });
  
  function deleteCard(key) {
    // Xóa thẻ khỏi Firebase sử dụng key
    firebaseRef1.child(key).remove();
  }
  
//=======================HISTORY CARD ===============================
var firebaseRef = firebase.database().ref("Card");
var outputContainer = document.querySelector('#HISTORYCard');
var cardRef = firebase.database().ref('Card');

firebaseRef.on("value", function(snapshot) {
  var output = '';
  var counter = 0;

  snapshot.forEach(function(parentSnapshot) {
    var parentKey = parentSnapshot.key;
    var childValues = [];

    parentSnapshot.forEach(function(childSnapshot) {
      var childValue = childSnapshot.val();
      childValues.push(childValue);
    });
    if (counter<=9){
        output += "<div>" + childValues.join(", ") + "</div>";
        counter++;
    }
    else{
        // Lấy tham chiếu đến thẻ con đầu tiên trong nút "Card"
        cardRef.orderByKey().limitToFirst(1).once('value')
        .then(function(snapshot) {
        var childKey = Object.keys(snapshot.val())[0]; // Lấy khóa của thẻ con đầu tiên
        var childRef = cardRef.child(childKey); // Tham chiếu đến thẻ con đầu tiên
        childRef.remove()
            .then(function() {
            console.log("Thẻ con đầu tiên đã được xóa thành công!");
            })
            .catch(function(error) {
            console.error("Xảy ra lỗi khi xóa thẻ con đầu tiên:", error);
            });
        })
        .catch(function(error) {
        console.error("Xảy ra lỗi khi truy vấn dữ liệu:", error);
        counter--;
        });
    }
  });

  // Xóa nội dung HTML cũ
  outputContainer.innerHTML = '';

  // Thêm nội dung HTML mới
  outputContainer.innerHTML = output;
});
