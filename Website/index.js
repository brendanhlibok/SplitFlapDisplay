const spin = document.getElementById('spin')

spin.addEventListener('click', commandMotor)

function commandMotor() {
    steps = document.querySelector('textarea')
    direction = document.querySelector('input[id="dir"]:checked');

    const data = {
      steps: steps.value,
      direction: direction.name
    };

    if (direction){
         console.log("Steps: ", steps.value);
         console.log("Direction: ", direction.name);

    } else {
        console.log("No radio button selected")
    }

    console.log(JSON.stringify(data));

    response = fetch("http://splitflapdisplay.local/test", {
      method: "POST",
      headers: {
        "Content-Type": "text/plain"
      },
      body: JSON.stringify(data),
    });
}
