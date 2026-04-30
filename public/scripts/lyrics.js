(async () => {
    const el = document.getElementById("lyrics");
    const structure = await fetch("./structure.json").then(res => res.json());
    let song = [];
    let current_lyric_index = -1;
    let time_start = Date.now();
    let duration = 0;
    let pre = "", cur = "", next = "";
    let looping = true;
    let animation_id;

    const random_song_name = () => structure[Math.floor(Math.random() * structure.length)];

    const split_time_and_text = (text_format) => {
        const match = text_format.match(/^\[(\d+):(\d+):(\d+):(\d+)\]\s*(.*)$/);
        if (match) {
            const [_, hh, mm, ss, ms, text] = match;
            return [
                Number(hh)*3600000 + Number(mm)*60000 + Number(ss)*1000 + Number(ms),
                text
            ];
        }
    };

    const reset = () => {
        time_start = Date.now();
        current_lyric_index = -1;
        pre = ""; cur = ""; next = song[0][1];
        draw();
    };
    
    const draw = () => {
        el.innerHTML = `
            <span style="color: grey;">${pre ?? ""}</span><br><br>
            <span>${cur ?? ""}</span><br><br>
            <span style="color: grey;">${next ?? ""}</span>
        `;
    };
    
    const update = () => {
        if (Date.now() - time_start > duration) {
            if (looping) reset();
            else {
                document.getElementById("song-name").textContent = "Currently not playing";
                cancelAnimationFrame(animation_id);
                return;
            }
        }
        else if (Date.now() - time_start >= song[current_lyric_index + 1]?.[0] ?? 999999999) {
            current_lyric_index++;
            pre = cur; cur = next; next = song[current_lyric_index + 1]?.[1] ?? "";
            draw();
        }
        animation_id = requestAnimationFrame(update);
    };

    const init_song = async (song_name) => {
        try {
            song = await fetch(`./songs/${song_name}.json`)
                        .then(res => res.json())
                        .then(data => {
                            duration = (([h, m, s, ms]) => h*3600000 + m*60000 + s*1000 + ms)(data.duration.split(':').map(Number));
                            document.getElementById("song-name").textContent = `Now playing: ${data.name}`;
                            return data.lyrics.map(lyric => split_time_and_text(lyric))
                        });
            reset();
            cancelAnimationFrame(animation_id);
            animation_id = requestAnimationFrame(update);
        } catch (err) {
            console.error(err);
        }
    };
    
    document.getElementById("next-song-btn").onclick = () => init_song(random_song_name());
    const loop_btn = document.getElementById("loop-btn");
    loop_btn.onclick = () => {
        looping = !looping;
        if (looping) loop_btn.firstChild.style.color = "white";
        else loop_btn.firstChild.style.color = "grey";
    }

    await init_song(random_song_name());
})();