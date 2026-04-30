(() => {

    const el = document.querySelector(".div4");

    // --- Measure character size based on element styles ---
    const measureChar = (el) => {
        const span = document.createElement("span");
        span.textContent = "M";

        const style = getComputedStyle(el);
        span.style.fontFamily = style.fontFamily;
        span.style.fontSize = style.fontSize;
        span.style.fontWeight = style.fontWeight;
        span.style.letterSpacing = style.letterSpacing;
        span.style.lineHeight = style.lineHeight;

        span.style.position = "absolute";
        span.style.visibility = "hidden";
        span.style.whiteSpace = "pre";

        document.body.appendChild(span);
        const rect = span.getBoundingClientRect();
        document.body.removeChild(span);

        return {
            width: rect.width,
            height: rect.height
        };
    };

    // --- Get char size ---
    let { width: charWidth, height: charHeight } = measureChar(el);

    // --- Buffer ---
    let buffer = [];
    let rows = 0;
    let columns = 0;

    // --- Resize buffer based on element size ---
    const resizeBuffer = () => {
        const rect = el.getBoundingClientRect();

        const newRows = Math.floor(rect.height / charHeight);
        const newCols = Math.floor(rect.width / charWidth);

        // Only rebuild if size actually changed
        if (newRows === rows && newCols === columns) return;

        rows = newRows;
        columns = newCols;

        buffer = Array.from({ length: rows }, () =>
            Array.from({ length: columns }, () => " ")
        );
    };

    // --- Observe element resize (works with flex layouts) ---
    const observer = new ResizeObserver(() => {
        resizeBuffer();
    });

    observer.observe(el);

    // --- Initial setup ---
    resizeBuffer();

    document.addEventListener("resize", resizeBuffer);

    const chars = "QWERTYUIOPASDFGHJKLZXCVBNM1234567890";
    const new_char_rate = 0.095;
    const end_trace_rate = 0.4;
    const random_char = () => chars[Math.floor(Math.random() * chars.length)];
    const update = () => {
        for (let i = buffer.length - 1; i >= 0; i--) {
            for(let j = 0; j < buffer[i].length; j++) {
                if (i == 0) {
                    // first line
                    if (buffer[0][j] === " ") {
                        if (Math.random() < new_char_rate) buffer[0][j] = random_char();
                    } else {
                        if (Math.random() < end_trace_rate) buffer[0][j] = " ";
                    }
                }
                else {
                    if (buffer[i - 1][j] !== " ") {
                        if (buffer[i][j] === " ")
                        buffer[i][j] = random_char();
                    } else {
                        buffer[i][j] = " ";
                    }
                }
            }
        }
    };
    const print = () => {
        el.textContent = buffer
            .map(row => row.join(""))   // convert row → string
            .join("\n");                // join rows
    };
    setInterval(() => {
        update(); print();
    }, 150);
})();