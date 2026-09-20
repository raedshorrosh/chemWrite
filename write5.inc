[[iframe width="100%" height="450px"]]

[[script src="https://unpkg.com/mathlive@0.110.0"]][[/script]]

[[style]]

math-field::part(virtual-keyboard-toggle),
math-field::part(menu-toggle) {
    display: none !important;
}

#mathfields-container label {
    background-color: #f4f8fa !important;
    color: #0073a5 !important;
    padding: 6px 16px !important;
    border-radius: 20px !important;
    border: 1px solid #b8daff !important;
    display: inline-block !important;
    width: fit-content !important;
    margin-bottom: 12px !important;
    margin-left: auto !important;
    margin-right: 0 !important;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05) !important;
    font-size: 1.1em !important;
    text-align: right !important;
    direction: rtl !important;
}

[[/style]]


<div style="display:none;">

    <div id="data-tas">{#js_tas#}</div>

    <div id="data-prefixes">{#js_prefixes#}</div>

    <div id="data-titles">{#js_titles#}</div>

    <div id="data-tex">{#js_tex#}</div>

    <div id="data-partial">{#js_partial_credit#}</div>

    <div id="data-errors">{#js_error_dict#}</div>

    <div id="data-palette-title">{#js_title#}</div>

    <div id="data-debug">{#debug_display#}</div>

</div>


<div
    style="
        font-family:sans-serif;
        padding:10px;
        box-sizing:border-box;
    "
>

    <div
        id="button-palette"
        style="
            background:white;
            border:1px solid #ced4da;
            border-radius:6px;
            margin-bottom:5px;
            overflow:hidden;
            width:fit-content;
            max-width:100%;
            box-shadow:0 4px 6px rgba(0,0,0,0.1);
        "
    >

        <div
            id="palette-title"
            dir="rtl"
            style="
                padding:8px 12px;
                background:#e9ecef;
                border-bottom:1px solid #ced4da;
                font-weight:bold;
                font-size:0.85em;
                color:#495057;
            "
        >
            Symbols & Templates
        </div>

        <div
            id="preset-buttons-container"
            style="
                display:flex;
                flex-wrap:wrap;
                gap:8px;
                padding:10px;
            "
        >
        </div>

    </div>


    <div
        id="mathfields-container"
        style="
            display:flex;
            flex-direction:column;
            gap:15px;
        "
    >
    </div>

</div>


[[script type="module"]]


/* =========================================================
   HIDE MATHLIVE ICONS
   ========================================================= */

const hideIconsStyle =
    document.createElement("style");

hideIconsStyle.textContent = `
    math-field::part(virtual-keyboard-toggle),
    math-field::part(menu-toggle) {
        display:none !important;
    }
`;

document.head.appendChild(
    hideIconsStyle
);



/* =========================================================
   STACK API
   ========================================================= */

import {stack_js}
from '[[cors src="stackjsiframe.js"/]]';



/* =========================================================
   STACK DATA HELPERS
   ========================================================= */

function getStackData(id) {

    const el =
        document.getElementById(id);

    if (!el) {
        return null;
    }

    let txt =
        el.innerText ||
        el.textContent ||
        "";

    txt =
        txt.trim();

    if (
        txt.includes("{#")
    ) {
        return null;
    }

    return txt;
}



function parseStackJSON(
    id,
    fallback
) {

    try {

        const raw =
            getStackData(id);

        if (!raw) {
            return fallback;
        }


        let parsed =
            JSON.parse(raw);


        if (
            typeof parsed ===
            "string"
        ) {

            try {

                parsed =
                    JSON.parse(parsed);

            }
            catch (e) {

                return [parsed];
            }
        }


        if (
            !Array.isArray(parsed)
        ) {

            parsed =
                [parsed];
        }


        return parsed;

    }
    catch (e) {

        console.warn(
            "STACK JSON error:",
            id,
            e
        );

        return fallback;
    }
}



function parseStackString(
    id,
    fallback = ""
) {

    let raw =
        getStackData(id);

    if (!raw) {
        return fallback;
    }


    try {

        if (
            raw.startsWith('"') &&
            raw.endsWith('"')
        ) {

            return JSON.parse(raw);
        }

    }
    catch (e) {}


    return raw.replace(
        /^["']|["']$/g,
        ""
    );
}



function renderMathInElementSafe(
    element
) {

    try {

        if (
            window.MathLive &&
            typeof window.MathLive
                .renderMathInElement ===
                "function"
        ) {

            window.MathLive
                .renderMathInElement(
                    element
                );
        }

    }
    catch (e) {}
}



/* =========================================================
   READ STACK VARIABLES
   ========================================================= */

let tas =
    parseStackJSON(
        "data-tas",
        []
    );


let prefixes =
    parseStackJSON(
        "data-prefixes",
        []
    );


let titles =
    parseStackJSON(
        "data-titles",
        []
    );


let texs =
    parseStackJSON(
        "data-tex",
        []
    );


let errorMessages =
    parseStackJSON(
        "data-errors",
        []
    );


let partialCredit =
    0.5;


try {

    const pc =
        parseFloat(
            getStackData(
                "data-partial"
            )
        );


    if (!isNaN(pc)) {

        partialCredit =
            pc;
    }

}
catch (e) {}



/* =========================================================
   PALETTE TITLE
   ========================================================= */

const paletteTitle =
    parseStackString(
        "data-palette-title",
        ""
    );


if (paletteTitle) {

    document.getElementById(
        "palette-title"
    ).innerText =
        paletteTitle;
}



/* =========================================================
   CHEMISTRY ENGINE
   ========================================================= */

const REACTION_ARROW_RE =
    /\\xrightarrow(?:\[[^\]]*\])?\{[^{}]*\}|\\rightleftharpoons|\\longrightarrow(?:\{\})?|\\rightarrow(?:\{\})?|\\Rightarrow(?:\{\})?|=/;



/* =========================================================
   BASIC LATEX CLEANING
   ========================================================= */

function cleanLatex(s) {

    s =
        String(
            s ?? ""
        );


    s =
        s.replace(
            /\\textcolor\{[^{}]*\}\{([^{}]*)\}/g,
            "$1"
        );


    let previous;


    do {

        previous =
            s;


        s =
            s.replace(
                /\\(?:mathbf|mathrm|mathit|mathsf|mathtt)\{([^{}]*)\}/g,
                "$1"
            );

    }
    while (
        s !== previous
    );


    return s

        .replace(
            /\\left/g,
            ""
        )

        .replace(
            /\\right/g,
            ""
        )

        .replace(
            /\\[,;!]/g,
            ""
        )

        .replace(
            /\s+/g,
            ""
        )

        .replace(
            /\{\}/g,
            ""
        );
}



/* =========================================================
   NORMALIZE DOUBLE SUBSCRIPT / SUPERSCRIPT

   Examples:

   C_3H_8_{(g)}
          ↓
   C_3H_{8(g)}

   C_3H_{8(g)}
          ↓
   C_3H_{8(g)}

   C_{3}H_8_{(g)}
          ↓
   C_3H_{8(g)}

   ========================================================= */

function normalizeScripts(s) {

    let result =
        "";

    let i =
        0;



    function readScriptArgument(
        start
    ) {

        if (
            start >= s.length
        ) {

            return {
                content: "",
                end: start
            };
        }



        /* ---------------------------------------------
           BRACED ARGUMENT
           --------------------------------------------- */

        if (
            s[start] === "{"
        ) {

            let depth =
                1;

            let j =
                start + 1;

            let content =
                "";


            while (
                j < s.length &&
                depth > 0
            ) {

                if (
                    s[j] === "{"
                ) {

                    depth++;

                    if (
                        depth > 1
                    ) {

                        content +=
                            s[j];
                    }

                }

                else if (
                    s[j] === "}"
                ) {

                    depth--;

                    if (
                        depth > 0
                    ) {

                        content +=
                            s[j];
                    }

                }

                else {

                    content +=
                        s[j];
                }


                j++;
            }


            return {

                content:
                    content,

                end:
                    j
            };
        }



        /* ---------------------------------------------
           LATEX COMMAND
           --------------------------------------------- */

        if (
            s[start] === "\\"
        ) {

            let j =
                start + 1;


            while (
                j < s.length &&
                /[A-Za-z]/.test(
                    s[j]
                )
            ) {

                j++;
            }


            return {

                content:
                    s.slice(
                        start,
                        j
                    ),

                end:
                    j
            };
        }



        /* ---------------------------------------------
           SINGLE CHARACTER
           --------------------------------------------- */

        return {

            content:
                s[start],

            end:
                start + 1
        };
    }



    while (
        i < s.length
    ) {

        const ch =
            s[i];


        if (
            ch !== "_" &&
            ch !== "^"
        ) {

            result +=
                ch;

            i++;

            continue;
        }



        const operator =
            ch;


        i++;


        let parsed =
            readScriptArgument(
                i
            );


        let combined =
            parsed.content;


        i =
            parsed.end;



        /*
         Merge consecutive scripts of the SAME kind.

         H_8_{(g)}

         becomes:

         H_{8(g)}
        */

        while (

            i < s.length &&

            s[i] === operator

        ) {

            i++;


            parsed =
                readScriptArgument(
                    i
                );


            combined +=
                parsed.content;


            i =
                parsed.end;
        }



        /*
         One-character script:

         _3  stays _3

         Longer script:

         _8_(g)

         becomes:

         _{8(g)}
        */

        if (
            combined.length === 1
        ) {

            result +=
                operator +
                combined;

        }

        else {

            result +=
                operator +
                "{" +
                combined +
                "}";
        }
    }


    return result;
}



/* =========================================================
   EXACT SPECIES NORMALIZATION
   ========================================================= */

function exactSpecies(s) {

    s =
        cleanLatex(
            s
        );


    s =
        normalizeScripts(
            s
        );


    return s;
}



/* =========================================================
   CORE SPECIES NORMALIZATION

   Used for detecting formatting errors.

   States and script formatting are ignored,
   but chemical capitalization is preserved.

   CO != Co
   ========================================================= */

function coreSpecies(s) {

    s =
        exactSpecies(
            s
        );


    /*
     Remove physical states even if they are
     contained inside a subscript.

     H_{2(g)}
        →
     H_{2}
    */

    s =
        s.replace(
            /\((?:aq|g|l|s)\)/g,
            ""
        );


    s =
        s

        .replace(
            /\\alpha/g,
            "α"
        )

        .replace(
            /\\beta/g,
            "β"
        )

        .replace(
            /\\gamma/g,
            "γ"
        )

        .replace(
            /\\omega/g,
            "ω"
        );


    /*
     Remove remaining LaTeX commands.
    */

    s =
        s.replace(
            /\\[A-Za-z]+/g,
            ""
        );


    /*
     Remove notation formatting,
     but keep numbers and charges.
    */

    s =
        s.replace(
            /[_^{}()[\]]/g,
            ""
        );


    return s;
}



/* =========================================================
   NORMALIZE ARROW
   ========================================================= */

function normalizeArrow(
    arrow
) {

    const s =
        cleanLatex(
            arrow
        );


    if (
        s.includes(
            "\\rightleftharpoons"
        )
    ) {

        return "<=>";
    }


    if (

        s.includes(
            "\\xrightarrow"
        )

        ||

        s.includes(
            "\\longrightarrow"
        )

        ||

        s.includes(
            "\\rightarrow"
        )

        ||

        s.includes(
            "\\Rightarrow"
        )

    ) {

        return "->";
    }


    if (
        s === "="
    ) {

        return "=";
    }


    return s;
}



/* =========================================================
   SPLIT REACTION INTO TERMS
   ========================================================= */

function splitTerms(
    side
) {

    const terms =
        [];


    let current =
        "";


    let braces =
        0;


    let parentheses =
        0;


    let brackets =
        0;



    for (
        let i = 0;
        i < side.length;
        i++
    ) {

        const ch =
            side[i];


        if (
            ch === "{"
        ) {

            braces++;

        }

        else if (
            ch === "}"
        ) {

            braces =
                Math.max(
                    0,
                    braces - 1
                );

        }

        else if (
            ch === "("
        ) {

            parentheses++;

        }

        else if (
            ch === ")"
        ) {

            parentheses =
                Math.max(
                    0,
                    parentheses - 1
                );

        }

        else if (
            ch === "["
        ) {

            brackets++;

        }

        else if (
            ch === "]"
        ) {

            brackets =
                Math.max(
                    0,
                    brackets - 1
                );
        }



        if (

            ch === "+" &&

            braces === 0 &&

            parentheses === 0 &&

            brackets === 0

        ) {

            const before =
                current.trim();


            /*
             Protect unbraced ionic charge:
             H^+
             Fe^3+
            */

            const chargePlus =
                /\^\d*$/.test(
                    before
                );


            if (
                !chargePlus
            ) {

                if (
                    before !== ""
                ) {

                    terms.push(
                        before
                    );
                }


                current =
                    "";


                continue;
            }
        }


        current +=
            ch;
    }



    if (
        current.trim() !==
        ""
    ) {

        terms.push(
            current.trim()
        );
    }


    return terms;
}



/* =========================================================
   PARSE TERM
   ========================================================= */

function parseTerm(
    term
) {

    let s =
        exactSpecies(
            term
        );


    let coefficient =
        1;


    let body =
        s;


    const match =
        s.match(
            /^(\d+)(.+)$/
        );


    if (
        match
    ) {

        coefficient =
            Number(
                match[1]
            );


        body =
            match[2];
    }


    return {

        coefficient:
            coefficient,

        exact:
            exactSpecies(
                body
            ),

        core:
            coreSpecies(
                body
            )
    };
}



/* =========================================================
   SIDE SIGNATURE
   ========================================================= */

function sideSignature(
    terms,
    mode,
    includeCoefficient
) {

    return terms

        .map(
            term => {

                if (
                    includeCoefficient
                ) {

                    return (
                        term.coefficient +
                        ":" +
                        term[mode]
                    );
                }


                return term[mode];
            }
        )

        .sort()

        .join("+");
}



/* =========================================================
   PROCESS CHEMICAL EXPRESSION
   ========================================================= */

function processChem(
    latex
) {

    latex =
        String(
            latex ?? ""
        );


    const arrowMatch =
        latex.match(
            REACTION_ARROW_RE
        );



    /* =====================================================
       SINGLE FORMULA
       ===================================================== */

    if (
        !arrowMatch
    ) {

        const term =
            parseTerm(
                latex
            );


        return {

            isReaction:
                false,

            parsedTerm:
                term,

            exact:
                term.coefficient +
                ":" +
                term.exact,

            core:
                term.coefficient +
                ":" +
                term.core,

            speciesExact:
                term.exact,

            speciesCore:
                term.core
        };
    }



    /* =====================================================
       COMPLETE REACTION
       ===================================================== */

    const arrowText =
        arrowMatch[0];


    const leftText =
        latex.slice(
            0,
            arrowMatch.index
        );


    const rightText =
        latex.slice(
            arrowMatch.index +
            arrowText.length
        );


    const left =
        splitTerms(
            leftText
        ).map(
            parseTerm
        );


    const right =
        splitTerms(
            rightText
        ).map(
            parseTerm
        );


    const arrow =
        normalizeArrow(
            arrowText
        );


    return {

        isReaction:
            true,

        left:
            left,

        right:
            right,

        arrow:
            arrow,


        exact:

            sideSignature(
                left,
                "exact",
                true
            )

            +

            "|" +
            arrow +
            "|"

            +

            sideSignature(
                right,
                "exact",
                true
            ),


        core:

            sideSignature(
                left,
                "core",
                true
            )

            +

            "|" +
            arrow +
            "|"

            +

            sideSignature(
                right,
                "core",
                true
            ),


        speciesExact:

            sideSignature(
                left,
                "exact",
                false
            )

            +

            "|" +
            arrow +
            "|"

            +

            sideSignature(
                right,
                "exact",
                false
            ),


        speciesCore:

            sideSignature(
                left,
                "core",
                false
            )

            +

            "|" +
            arrow +
            "|"

            +

            sideSignature(
                right,
                "core",
                false
            )
    };
}



/* =========================================================
   STOICHIOMETRIC MAP
   ========================================================= */

function makeStoichiometricMap(
    reaction,
    mode
) {

    const map =
        new Map();


    function add(
        side,
        term
    ) {

        const key =
            side +
            "|" +
            term[mode];


        map.set(

            key,

            (
                map.get(key) ||
                0
            )

            +

            term.coefficient
        );
    }


    reaction.left.forEach(
        term =>
            add(
                "L",
                term
            )
    );


    reaction.right.forEach(
        term =>
            add(
                "R",
                term
            )
    );


    return map;
}



/* =========================================================
   TEST WHETHER COEFFICIENTS ARE PROPORTIONAL

   Example:

   Teacher:
   2C2H6 + 7O2 -> 4CO2 + 6H2O

   Student:
   4C2H6 + 14O2 -> 8CO2 + 12H2O

   Result:
   correct
   ========================================================= */

function coefficientsAreProportional(
    student,
    teacher,
    mode
) {

    if (

        !student.isReaction ||

        !teacher.isReaction

    ) {

        return false;
    }


    if (
        student.arrow !==
        teacher.arrow
    ) {

        return false;
    }


    const sm =
        makeStoichiometricMap(
            student,
            mode
        );


    const tm =
        makeStoichiometricMap(
            teacher,
            mode
        );


    if (
        sm.size !==
        tm.size
    ) {

        return false;
    }


    let baseStudent =
        null;


    let baseTeacher =
        null;



    for (
        const [
            key,
            teacherCoefficient
        ]
        of tm
    ) {

        if (
            !sm.has(key)
        ) {

            return false;
        }


        const studentCoefficient =
            sm.get(key);


        if (
            baseStudent ===
            null
        ) {

            baseStudent =
                studentCoefficient;


            baseTeacher =
                teacherCoefficient;

        }

        else {

            if (

                studentCoefficient *
                baseTeacher

                !==

                teacherCoefficient *
                baseStudent

            ) {

                return false;
            }
        }
    }


    return true;
}



/* =========================================================
   PREPROCESS TEACHER ANSWERS
   ========================================================= */

const teacherDataArray =
    tas.map(
        ta =>
            processChem(
                String(ta)
            )
    );



/* =========================================================
   UI
   ========================================================= */

const btnContainer =
    document.getElementById(
        "preset-buttons-container"
    );


const container =
    document.getElementById(
        "mathfields-container"
    );


let mfs =
    [];


let activeMathField =
    null;



/* =========================================================
   CREATE MATH FIELDS
   ========================================================= */

tas.forEach(
    (ta, index) => {


        const wrapper =
            document.createElement(
                "div"
            );


        wrapper.setAttribute(
            "dir",
            "rtl"
        );


        wrapper.style.cssText = `
            border:1px solid #ced4da;
            padding:15px;
            border-radius:4px;
            background:white;
            display:flex;
            flex-direction:column;
            position:relative;
        `;



        if (
            titles[index]
        ) {

            const label =
                document.createElement(
                    "label"
                );


            label.setAttribute(
                "dir",
                "rtl"
            );


            label.style.cssText = `
                background-color:#f4f8fa;
                color:#0073a5;
                padding:6px 16px;
                border-radius:20px;
                border:1px solid #b8daff;
                display:inline-block;
                width:fit-content;
                margin-bottom:12px;
                margin-left:auto;
                margin-right:0;
                box-shadow:0 2px 4px rgba(0,0,0,0.05);
                font-size:1.1em;
                text-align:right;
                direction:rtl;
                font-weight:bold;
            `;


            label.innerHTML =
                titles[index];


            wrapper.appendChild(
                label
            );


            renderMathInElementSafe(
                label
            );
        }



        const mfRow =
            document.createElement(
                "div"
            );


        mfRow.style.cssText = `
            display:flex;
            align-items:center;
            width:100%;
        `;



        const mf =
            document.createElement(
                "math-field"
            );


        mf.setAttribute(
            "dir",
            "ltr"
        );


        mf.style.cssText = `
            width:100%;
            font-size:1.4em;
            background-color:#ffffff;
            border:2px solid #93c5fd;
            border-radius:8px;
            padding:12px;
            box-sizing:border-box;
            box-shadow:0 2px 4px rgba(0,0,0,0.05);
            transition:all 0.2s ease;
        `;



        mf.addEventListener(
            "focusin",
            () => {

                activeMathField =
                    mf;


                mf.style.boxShadow =
                    "0 0 0 4px rgba(147,197,253,0.4)";


                const palette =
                    document.getElementById(
                        "button-palette"
                    );


                if (
                    palette &&
                    wrapper.parentNode
                ) {

                    wrapper.parentNode
                        .insertBefore(
                            palette,
                            wrapper
                        );
                }
            }
        );



        mf.addEventListener(
            "focusout",
            () => {

                mf.style.boxShadow =
                    "0 2px 4px rgba(0,0,0,0.05)";
            }
        );



        mfRow.appendChild(
            mf
        );


        wrapper.appendChild(
            mfRow
        );


        container.appendChild(
            wrapper
        );


        mfs.push({

            element:
                mf,

            row:
                mfRow
        });

    }
);



if (
    mfs.length > 0
) {

    activeMathField =
        mfs[0].element;
}



/* =========================================================
   PALETTE BUTTONS
   ========================================================= */

for (
    let i = 0;
    i < texs.length;
    i++
) {

    let displayTex;

    let insertTex;



    if (
        Array.isArray(
            texs[i]
        )
    ) {

        displayTex =
            texs[i][0];


        insertTex =
            texs[i][1] ||
            texs[i][0];

    }

    else {

        displayTex =
            texs[i];


        insertTex =
            texs[i];
    }



    const btn =
        document.createElement(
            "div"
        );


    btn.style.cssText = `
        position:relative;
        cursor:pointer;
        width:85px;
        height:45px;
        box-sizing:border-box;
        border:1px solid #ced4da;
        border-radius:4px;
        background-color:#f8f9fa;
        display:flex;
        align-items:center;
        justify-content:center;
        user-select:none;
        transition:all 0.2s;
    `;



    btn.addEventListener(
        "mouseenter",
        () => {

            btn.style.backgroundColor =
                "#e2e6ea";


            btn.style.boxShadow =
                "0 2px 4px rgba(0,0,0,0.1)";
        }
    );



    btn.addEventListener(
        "mouseleave",
        () => {

            btn.style.backgroundColor =
                "#f8f9fa";


            btn.style.boxShadow =
                "none";
        }
    );



    const mathHtml = `
        <span
            style="
                font-size:1.2em;
                color:black;
                pointer-events:none;
            "
        >
            \\( ${displayTex} \\)
        </span>
    `;



    const overlayHtml = `
        <div
            style="
                position:absolute;
                top:0;
                left:0;
                right:0;
                bottom:0;
                z-index:100;
            "
        >
        </div>
    `;



    btn.innerHTML =
        mathHtml +
        overlayHtml;


    renderMathInElementSafe(
        btn
    );



    btn.addEventListener(
        "click",
        e => {

            e.preventDefault();

            e.stopPropagation();


            if (
                !activeMathField
            ) {

                return;
            }



            if (
                insertTex === "^"
            ) {

                activeMathField
                    .executeCommand(
                        "moveToSuperscript"
                    );

            }

            else if (
                insertTex === "_"
            ) {

                activeMathField
                    .executeCommand(
                        "moveToSubscript"
                    );

            }

            else {

                activeMathField.insert(
                    insertTex
                );
            }


            activeMathField.focus();
        }
    );


    btnContainer.appendChild(
        btn
    );
}



/* =========================================================
   STACK INPUTS
   ========================================================= */

Promise.all([

    stack_js.request_access_to_input(
        "ans1",
        true
    ),

    stack_js.request_access_to_input(
        "ans2",
        true
    )

])

.then(
    ([inputId1, inputId2]) => {


        const hiddenInput1 =
            document.getElementById(
                inputId1
            );


        const hiddenInput2 =
            document.getElementById(
                inputId2
            );


        if (
            !hiddenInput1 ||
            !hiddenInput2
        ) {

            console.error(
                "STACK inputs not found."
            );

            return;
        }



        const isLocked =

            hiddenInput1.readOnly ||

            hiddenInput1.disabled ||

            hiddenInput1.hasAttribute(
                "readonly"
            ) ||

            hiddenInput1.hasAttribute(
                "disabled"
            );



        /* =================================================
           STUDENT EVALUATION
           ================================================= */

        function evaluateStudent() {

            if (
                isLocked
            ) {

                return;
            }


            let currentAnswers =
                [];


            let currentGrades =
                [];


            let currentErrors =
                [];



            mfs.forEach(
                (mfObj, index) => {


                    const rawLatex =
                        mfObj.element
                            .getValue(
                                "latex"
                            ) || "";


                    currentAnswers.push(
                        rawLatex
                    );



                    if (
                        rawLatex.trim() ===
                        ""
                    ) {

                        currentGrades.push(
                            0
                        );


                        currentErrors.push(
                            ""
                        );


                        return;
                    }



                    const studentData =
                        processChem(
                            rawLatex
                        );


                    const tData =
                        teacherDataArray[
                            index
                        ];


                    let grade =
                        0;


                    let currentInputErrors =
                        [];


                    let isUnbalanced =
                        false;


                    let isBadFormat =
                        false;



                    /* =====================================
                       EXACT ANSWER
                       ===================================== */

                    if (

                        studentData.exact ===
                        tData.exact

                    ) {

                        grade =
                            1;
                    }



                    /* =====================================
                       PROPORTIONAL BALANCED REACTION
                       ===================================== */

                    else if (

                        studentData.isReaction &&

                        tData.isReaction &&

                        coefficientsAreProportional(
                            studentData,
                            tData,
                            "exact"
                        )

                    ) {

                        grade =
                            1;
                    }



                    /* =====================================
                       CORRECT SPECIES
                       WRONG COEFFICIENTS
                       ===================================== */

                    else if (

                        studentData.isReaction &&

                        tData.isReaction &&

                        studentData.speciesExact ===
                        tData.speciesExact

                    ) {

                        isUnbalanced =
                            true;


                        grade =
                            partialCredit;
                    }



                    /* =====================================
                       BALANCED BUT FORMATTING / STATES WRONG
                       ===================================== */

                    else if (

                        studentData.isReaction &&

                        tData.isReaction &&

                        coefficientsAreProportional(
                            studentData,
                            tData,
                            "core"
                        )

                    ) {

                        isBadFormat =
                            true;


                        grade =
                            partialCredit;
                    }



                    /* =====================================
                       SAME BASIC SPECIES BUT BOTH
                       COEFFICIENTS AND FORMAT WRONG
                       ===================================== */

                    else if (

                        studentData.isReaction &&

                        tData.isReaction &&

                        studentData.speciesCore ===
                        tData.speciesCore

                    ) {

                        isUnbalanced =
                            true;


                        isBadFormat =
                            true;


                        grade =
                            0;
                    }



                    /* =====================================
                       SINGLE COMPOUND:
                       CORRECT CORE, BAD FORMAT
                       ===================================== */

                    else if (

                        !studentData.isReaction &&

                        !tData.isReaction &&

                        studentData.speciesCore ===
                        tData.speciesCore

                    ) {

                        isBadFormat =
                            true;


                        grade =
                            partialCredit;
                    }



                    /* =====================================
                       WRONG CHEMISTRY
                       ===================================== */

                    else {

                        currentInputErrors.push(

                            errorMessages[2]

                            ||

                            "הנוסחה לא נכונה"
                        );
                    }



                    if (
                        isUnbalanced
                    ) {

                        currentInputErrors.push(

                            errorMessages[0]

                            ||

                            "התגובה לא מאוזנת נכון"
                        );
                    }



                    if (
                        isBadFormat
                    ) {

                        currentInputErrors.push(

                            errorMessages[1]

                            ||

                            "בעיות במצבי צבירה או פורמט"
                        );
                    }



                    currentGrades.push(
                        grade
                    );


                    currentErrors.push(

                        currentInputErrors
                            .join(
                                "<br>"
                            )
                    );

                }
            );



            const newAnsStr =
                JSON.stringify(
                    currentAnswers
                );


            const newGradeStr =
                JSON.stringify([

                    currentGrades,

                    currentErrors

                ]);



            if (
                hiddenInput1.value !==
                newAnsStr
            ) {

                hiddenInput1.value =
                    newAnsStr;


                hiddenInput1
                    .dispatchEvent(

                        new Event(
                            "change",
                            {
                                bubbles:true
                            }
                        )
                    );
            }



            if (
                hiddenInput2.value !==
                newGradeStr
            ) {

                hiddenInput2.value =
                    newGradeStr;


                hiddenInput2
                    .dispatchEvent(

                        new Event(
                            "change",
                            {
                                bubbles:true
                            }
                        )
                    );
            }
        }



        /* =================================================
           RESTORE SAVED ANSWERS
           ================================================= */

        let savedAnswers =
            [];


        if (

            hiddenInput1.value &&

            hiddenInput1.value.trim() !==
            ""

        ) {

            try {

                let parsed =
                    JSON.parse(
                        hiddenInput1.value
                    );


                if (
                    typeof parsed ===
                    "string"
                ) {

                    parsed =
                        JSON.parse(
                            parsed
                        );
                }


                if (
                    Array.isArray(
                        parsed
                    )
                ) {

                    savedAnswers =
                        parsed;
                }

            }
            catch (e) {}
        }



        /* =================================================
           LOAD PREFIXES / SAVED VALUES
           ================================================= */

        mfs.forEach(
            (mfObj, index) => {


                if (

                    savedAnswers[index] !==
                    undefined &&

                    savedAnswers[index] !==
                    null &&

                    savedAnswers[index] !==
                    ""

                ) {

                    mfObj.element
                        .setValue(

                            savedAnswers[
                                index
                            ],

                            {
                                suppressChangeNotifications:
                                    true
                            }
                        );

                }

                else if (
                    prefixes[index]
                ) {

                    mfObj.element
                        .setValue(

                            prefixes[
                                index
                            ],

                            {
                                suppressChangeNotifications:
                                    true
                            }
                        );
                }



                let debounceTimer;



                mfObj.element
                    .addEventListener(
                        "input",
                        () => {

                            clearTimeout(
                                debounceTimer
                            );


                            debounceTimer =
                                setTimeout(
                                    () => {

                                        evaluateStudent();

                                        updateTeacherBox();

                                    },
                                    400
                                );
                        }
                    );

            }
        );



        if (
            !isLocked
        ) {

            evaluateStudent();

            updateTeacherBox();
        }



        /* =================================================
           POST-SUBMISSION
           ================================================= */

        if (
            isLocked
        ) {


            btnContainer.style.pointerEvents =
                "none";


            btnContainer.style.opacity =
                "0.6";


            let finalGrades =
                [];


            let finalErrors =
                [];



            try {

                let parsedAns2 =
                    JSON.parse(
                        hiddenInput2.value
                    );


                if (

                    Array.isArray(
                        parsedAns2
                    ) &&

                    Array.isArray(
                        parsedAns2[0]
                    )

                ) {

                    finalGrades =
                        parsedAns2[0];


                    finalErrors =
                        parsedAns2[1] ||
                        [];

                }

                else {

                    finalGrades =
                        parsedAns2;
                }

            }
            catch (e) {}



            mfs.forEach(
                (mfObj, index) => {


                    mfObj.element.readOnly =
                        true;


                    const grade =
                        Number(
                            finalGrades[
                                index
                            ]
                        );


                    const errorText =
                        finalErrors[
                            index
                        ];



                    if (
                        !isNaN(
                            grade
                        )
                    ) {


                        const markIcon =
                            document.createElement(
                                "div"
                            );


                        markIcon.style.cssText = `
                            margin-left:15px;
                            display:flex;
                            align-items:center;
                        `;



                        const svgCheck = `
                            <svg
                                viewBox="0 0 512 512"
                                width="24"
                                height="24"
                                fill="#4caf50"
                            >
                                <path
                                    d="M470.6 105.4c12.5 12.5 12.5 32.8 0 45.3l-256 256c-12.5 12.5-32.8 12.5-45.3 0l-128-128c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0L192 338.7 425.4 105.4c12.5-12.5 32.8-12.5 45.3 0z"
                                />
                            </svg>
                        `;



                        const svgAdjust = `
                            <svg
                                viewBox="0 0 512 512"
                                width="24"
                                height="24"
                                fill="#ff9800"
                            >
                                <path
                                    d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zM256 464c-114.7 0-208-93.3-208-208S141.3 48 256 48v416z"
                                />
                            </svg>
                        `;



                        const svgTimes = `
                            <svg
                                viewBox="0 0 320 512"
                                width="24"
                                height="24"
                                fill="#f44336"
                            >
                                <path
                                    d="M310.6 150.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L160 210.7 54.6 105.4c-12.5-12.5-32.8-12.5-45.3 0s-12.5 32.8 0 45.3L114.7 256 9.4 361.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 301.3 265.4 406.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L205.3 256 310.6 150.6z"
                                />
                            </svg>
                        `;



                        if (
                            grade === 1
                        ) {

                            markIcon.innerHTML =
                                svgCheck;

                        }

                        else if (

                            grade > 0 &&

                            grade < 1

                        ) {

                            markIcon.innerHTML =
                                svgAdjust;

                        }

                        else {

                            markIcon.innerHTML =
                                svgTimes;
                        }



                        mfObj.row
                            .appendChild(
                                markIcon
                            );



                        if (

                            errorText &&

                            errorText !==
                            ""

                        ) {

                            const errorDiv =
                                document.createElement(
                                    "div"
                                );


                            errorDiv.style.cssText = `
                                color:#d32f2f;
                                font-weight:bold;
                                margin-top:10px;
                                direction:rtl;
                                text-align:right;
                                background-color:#ffebee;
                                padding:6px 12px;
                                border-radius:6px;
                                border:1px solid #ffcdd2;
                                font-size:0.95em;
                            `;


                            errorDiv.innerHTML =
                                errorText;


                            mfObj.row
                                .parentElement
                                .appendChild(
                                    errorDiv
                                );
                        }
                    }

                }
            );
        }

    }
)

.catch(
    error => {

        console.error(
            "STACK initialization failed:",
            error
        );
    }
);



/* =========================================================
   TEACHER DEBUG BOX
   ========================================================= */

function updateTeacherBox() {

    try {

        const teacherBox =
            window.parent.document
                .getElementById(
                    "teacher-box"
                );


        if (
            !teacherBox
        ) {

            return;
        }


        const currentAnswers =
            mfs.map(
                mf =>
                    mf.element.getValue(
                        "latex"
                    )
            );


        const maximaArray =
            JSON.stringify(
                currentAnswers
            );


        const newText =

            "👨‍🏫 Teacher Maxima Code (Copy & Paste):\n\n"

            +

            "ta: "

            +

            maximaArray

            +

            ";";


        if (
            teacherBox.innerText !==
            newText
        ) {

            teacherBox.innerText =
                newText;
        }

    }
    catch (e) {}
}



/* =========================================================
   AUTO RESIZE
   ========================================================= */

function autoResize() {

    try {

        if (
            typeof FRAME_ID ===
            "undefined"
        ) {

            return;
        }


        const parentIframe =
            window.parent.document
                .getElementById(
                    FRAME_ID
                );


        const parentHolder =
            window.parent.document
                .getElementById(

                    FRAME_ID.replace(
                        "stack-iframe",
                        "stack-iframe-holder"
                    )
                );


        if (
            parentIframe &&
            parentHolder
        ) {

            const newHeight =

                (
                    document.body
                        .scrollHeight +
                    20
                )

                +

                "px";


            parentIframe.style.height =
                newHeight;


            parentHolder.style.height =
                newHeight;
        }

    }
    catch (e) {}
}



autoResize();



const resizeObserver =
    new ResizeObserver(
        () => {

            autoResize();
        }
    );


resizeObserver.observe(
    document.body
);


[[/script]]

[[/iframe]]
