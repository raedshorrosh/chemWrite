v=5
[[iframe width="100%" height="450px"]]
[[script src="https://unpkg.com/mathlive"]][[/script]]
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

<div style="display: none;">
    <div id="data-tas">{#js_tas#}</div>
    <div id="data-prefixes">{#js_prefixes#}</div>
    <div id="data-titles">{#js_titles#}</div>
    <div id="data-tex">{#js_tex#}</div>
    <div id="data-partial">{#js_partial_credit#}</div>
    <div id="data-errors" style="display: none;">{#js_error_dict#}</div>
</div>

<div style="font-family: sans-serif; padding: 10px; box-sizing: border-box;">
    <div id="button-palette" style="background: white; border: 1px solid #ced4da; border-radius: 6px; margin-bottom: 5px; overflow: hidden; width: fit-content; max-width: 100%; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
        <div id="palette-title" dir="rtl" style="padding: 8px 12px; background: #e9ecef; border-bottom: 1px solid #ced4da; font-weight: bold; font-size: 0.85em; color: #495057;">
            Symbols & Templates
        </div>
        <div id="preset-buttons-container" style="display: flex; flex-wrap: wrap; gap: 8px; padding: 10px;"></div>
    </div>
    
    <div id="mathfields-container" style="display: flex; flex-direction: column; gap: 15px;"></div>
</div>

[[script type="module"]]
import {stack_js} from '[[cors src="stackjsiframe.js"/]]';

// --- Data Parsing Helpers ---
function getStackData(id) {
    let el = document.getElementById(id);
    if (!el) return null;
    let txt = el.innerText || el.textContent;
    if (txt.includes('{#')) return null; 
    return txt.trim();
}

function parseStackJSON(id, fallback) {
    try {
        let rawData = getStackData(id);
        if (!rawData) return fallback;
        let parsed = JSON.parse(rawData);
        if (typeof parsed === 'string') parsed = JSON.parse(parsed);
        return Array.isArray(parsed) ? parsed : [parsed];
    } catch (e) { return fallback; }
}

// --- Init Variables ---
const tas = parseStackJSON('data-tas', []);
const prefixes = parseStackJSON('data-prefixes', []);
const titles = parseStackJSON('data-titles', []);
const texs = parseStackJSON('data-tex', []);
const errorMessages = parseStackJSON('data-errors', ["התגובה לא מאוזנת נכון", "בעיות בפורמט", "הנוסחה לא נכונה"]);
let partialCredit = parseFloat(getStackData('data-partial')) || 0.5;

const btnContainer = document.getElementById('preset-buttons-container');
const container = document.getElementById('mathfields-container');
let mfs = []; 
let activeMathField = null; 

// --- Core Chemistry Logic ---
function processChem(latex) {
    let arrowRegex = /((?:\\longrightarrow|\\xrightarrow|\\rightarrow|\\rightleftharpoons|\\Rightarrow)|=)/;
    let parts = latex.split(arrowRegex);
    if (parts.length !== 3) {
        let s = latex.replace(/\s+/g, '').toLowerCase();
        return { exact: s, sloppy: s, species: s };
    }
    
    let leftSide = parts[0], rightSide = parts[2], theArrow = parts[1].replace(/\s+/g, '');
    
    function clean(side, mode) {
        let terms = side.split('+').map(t => t.trim()).filter(t => t !== "");
        return terms.map(t => {
            let res = t.replace(/\s+/g, '').replace(/\\left|\\right|[\{\}]/g, '');
            if (mode === 'species') res = res.replace(/^[0-9]+/, ''); // Remove coefficients
            if (mode === 'sloppy') res = res.replace(/\\[a-zA-Z]+/g, '').replace(/[_^{}\(\)\[\]]/g, '').toLowerCase();
            return res;
        }).sort().join("+");
    }

    return {
        exact: clean(leftSide, 'exact') + theArrow + clean(rightSide, 'exact'),
        species: clean(leftSide, 'species') + theArrow + clean(rightSide, 'species'),
        sloppy: clean(leftSide, 'sloppy') + "ARR" + clean(rightSide, 'sloppy')
    };
}

const teacherDataArray = tas.map(ta => processChem(ta.replace(/\\\\/g, '\\')));

// --- UI Construction ---
tas.forEach((ta, index) => {
    const wrapper = document.createElement('div');
    wrapper.style.cssText = "border: 1px solid #ced4da; padding: 15px; border-radius: 4px; background: white; display: flex; flex-direction: column;";
    
    if (titles[index]) {
        const label = document.createElement('label');
        label.innerHTML = titles[index];
        wrapper.appendChild(label);
        if (window.MathLive) MathLive.renderMathInElement(label);
    }

    const mf = document.createElement('math-field');
    mf.style.cssText = "width: 100%; font-size: 1.4em; border: 2px solid #93c5fd; border-radius: 8px; padding: 10px;";
    mf.addEventListener('focusin', () => {
        activeMathField = mf;
        const palette = document.getElementById('button-palette');
        wrapper.parentNode.insertBefore(palette, wrapper);
    });

    const mfRow = document.createElement('div');
    mfRow.style.display = "flex";
    mfRow.style.alignItems = "center";
    mfRow.appendChild(mf);
    wrapper.appendChild(mfRow);
    container.appendChild(wrapper);
    mfs.push({ element: mf, row: mfRow, wrapper: wrapper });
});

// --- Button Palette Setup ---
texs.forEach(tex => {
    const btn = document.createElement('div');
    btn.style.cssText = "cursor: pointer; padding: 8px; border: 1px solid #ccc; background: #eee; min-width: 40px; text-align: center;";
    btn.innerHTML = `\\(${tex}\\)`;
    if (window.MathLive) MathLive.renderMathInElement(btn);
    btn.addEventListener('click', () => {
        if (activeMathField) { activeMathField.insert(tex); activeMathField.focus(); }
    });
    btnContainer.appendChild(btn);
});

// --- STACK Integration ---
stack_js.request_access_to_input("ans1", true).then((id1) => {
    const hidden1 = document.getElementById(id1);
    stack_js.request_access_to_input("ans2", true).then((id2) => {
        const hidden2 = document.getElementById(id2);
        const isLocked = hidden1.disabled || hidden1.readOnly;

        function evaluate() {
            if (isLocked) return;
            let answers = mfs.map(m => m.element.getValue("latex").replace(/\\/g, '\\\\'));
            let grades = mfs.map((m, i) => {
                let sData = processChem(m.element.getValue("latex"));
                let tData = teacherDataArray[i];
                if (sData.exact === tData.exact) return 1;
                if (sData.species === tData.species || sData.sloppy === tData.sloppy) return partialCredit;
                return 0;
            });
            hidden1.value = JSON.stringify(answers);
            hidden2.value = JSON.stringify(grades);
            hidden1.dispatchEvent(new Event('change'));
            hidden2.dispatchEvent(new Event('change'));
        }

        // Initialize values
        let saved = parseStackJSON(id1, []);
        mfs.forEach((m, i) => {
            if (saved[i]) m.element.setValue(saved[i].replace(/\\\\/g, '\\'), {suppressChangeNotifications: true});
            m.element.addEventListener('input', evaluate);
        });

        // Review Mode Logic
        if (isLocked) {
            let finalGrades = parseStackJSON(id2, []);
            mfs.forEach((m, i) => {
                m.element.readOnly = true;
                let grade = finalGrades[i];
                let sData = processChem(m.element.getValue("latex"));
                let tData = teacherDataArray[i];
        
               // --- Premium SVG Icons ---
               const svgCheck = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#4caf50"><path d="M470.6 105.4c12.5 12.5 12.5 32.8 0 45.3l-256 256c-12.5 12.5-32.8 12.5-45.3 0l-128-128c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0L192 338.7 425.4 105.4c12.5-12.5 32.8-12.5 45.3 0z"/></svg>`;
               const svgAdjust = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#ff9800"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zM256 464c-114.7 0-208-93.3-208-208S141.3 48 256 48v416z"/></svg>`;
               const svgTimes = `<svg viewBox="0 0 320 512" width="24" height="24" fill="#f44336"><path d="M310.6 150.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L160 210.7 54.6 105.4c-12.5-12.5-32.8-12.5-45.3 0s-12.5 32.8 0 45.3L114.7 256 9.4 361.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 301.3 265.4 406.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L205.3 256 310.6 150.6z"/></svg>`;

                let mark = document.createElement('div');
                mark.style.marginLeft = "15px";
                mark.style.display = "flex";
                mark.style.alignItems = "center";
    
                if (grade === 1) mark.innerHTML = svgCheck; 
                else if (grade > 0) mark.innerHTML = svgAdjust; 
                else mark.innerHTML = svgTimes; 

                m.row.appendChild(mark);
               

                // Feedback Boxes
                if (grade < 1 && m.element.getValue().trim() !== "") {
                    let errBox = document.createElement('div');
                    errBox.style.cssText = "color: red; background: #fff0f0; padding: 5px; margin-top: 5px; direction: rtl;";
                    if (sData.species === tData.species) errBox.innerText = errorMessages[0];
                    else if (sData.sloppy === tData.sloppy) errBox.innerText = errorMessages[1];
                    else errBox.innerText = errorMessages[2];
                    m.wrapper.appendChild(errBox);
                }
            });
        } else {
            evaluate();
        }
    });
});
[[/script]]
[[/iframe]]
