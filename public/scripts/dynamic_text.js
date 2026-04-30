(() => {
    const pretext = "Bui Duy Ninh | ";
    const dynamic_text = [
        "Bach Khoa Ha Noi",
        "HUST",
        "Ky thuat may tinh",
        "IT2",
        "Computer engineering",
        "Rudnin"
    ];
    const el = document.getElementById("dynamic-text");
    let text_index = 0, char_index = 0;
    let to_right = true; // char_index increment
    let last_update = 0;
    let pausing_duration = 100;

    const draw = () => {
        el.textContent = pretext + dynamic_text[text_index].slice(0, char_index);
    };

    const update = () => {
        if (Date.now() - last_update >= pausing_duration){ 
            if (to_right) {
                char_index++;
                if (char_index === dynamic_text[text_index].length) {
                    to_right = false;
                    pausing_duration = 1000;
                } else pausing_duration = 100;
            } else {
                char_index--;
                pausing_duration = 100;
                if (char_index === 0) {
                    to_right = true;
                    text_index = (text_index + 1) % dynamic_text.length;
                }
            }
            last_update = Date.now();
            draw(); 
        }
        requestAnimationFrame(update);
    }

    requestAnimationFrame(update);
})();