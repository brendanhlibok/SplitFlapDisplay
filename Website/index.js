const spin = document.getElementById('spin')

spin.addEventListener('click', commandMotor)

function commandMotor() {
    steps = document.querySelector('textarea')
    if (selectedRadio){
        direction = document.querySelector('input[name="dir"]:checked')
    } else {
        console.log("No radio button selected")
    }


}