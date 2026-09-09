const spin = document.getElementById('spin')

spin.addEventListener('click', commandMotor)

function commandMotor() {
    steps = document.querySelector('textarea')
    direction = document.querySelector('input[id="dir"]:checked');
    if (direction){
         console.log("Steps: ", steps.value);
         console.log("Direction: ", direction.name);

    } else {
        console.log("No radio button selected")
    }
    fetch("SplitFlapDisplay.local/test")

}