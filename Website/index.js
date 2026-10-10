const spin = document.getElementById('spin')

const rotAngle = document.getElementById("Rotation Angles")

rotAngle.addEventListener('change', setAngle)

spin.addEventListener('click', commandMotor)

function setAngle() {
    if (rotAngle.value != "custom"){
    document.querySelector('textarea').value = String(Math.floor(parseFloat(rotAngle.value) * 2038))
    } else {
      document.querySelector('textarea').value = ""
    }
}

function commandMotor() {
    steps = document.querySelector('textarea')
    direction = document.querySelector('input[id="dir"]:checked');

    const data = {
      steps: steps.value,
      direction: direction.value
    }

    if (direction){
         console.log("Steps: ", steps.value);
         console.log("Direction: ", direction.value);

    } else {
        console.log("No radio button selected")
    }

    console.log(JSON.stringify(data));

    response = fetch("http://splitflapdisplay.local/motor_command", {
      method: "POST",
      headers: {
        "Content-Type": "text/plain"
      },
      body: JSON.stringify(data),
    })
}
