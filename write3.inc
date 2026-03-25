v=6
[[iframe width="100%" height="550px"]]
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
        if (!Array.isArray(parsed)) parsed = [parsed];
        return parsed;
    } catch (e) { return fallback; }
}

let tas = parseStackJSON('data-tas', []);
let prefixes = parseStackJSON('data-prefixes', []);
let titles = parseStackJSON('data-titles', []);
let texs = parseStackJSON('data-tex', []);
let errorMessages = parseStackJSON('data-errors', ["התגובה לא מאוזנת נכון", "בעיות במצבי צבירה או פורמט", "הנוסחה לא נכונה"]);
let partialCredit = parseFloat(getStackData('data-partial')) || 0.5;

const container = document.getElementById('mathfields-container');
const btnContainer = document.getElementById('preset-buttons-container');
let mfs = []; 
let activeMathField = null; 

// --- RESTORED: Your Original Complex Chemistry Logic ---
function processChem(latex) {
    latex = latex.replace(/\\textcolor\{([^}]+)\}/g, 'COLOR$1');
    let arrowRegex = /((?:\\longrightarrow|\\xrightarrow|\\rightarrow|\\rightleftharpoons|\\Rightarrow)(?:_\{.*?\}|\^\{.*?\}|\{.*?\}|\[.*?\])*|=)/;
    let parts = latex.split(arrowRegex);
    
    if (parts.length !== 3) {
        let s = latex.replace(/\s+/g, '').toLowerCase();
        return { exact: s, sloppy: s, species: s, superSloppy: s }; 
    }
    
    let leftSide = parts[0], rightSide = parts[2], theArrow = parts[1].replace(/\s+/g, '').replace(/\{\}/g, ''); 
    
    function splitTerms(sideStr) {
        let terms = [], currentTerm = "", depth = 0;
        for (let i = 0; i < sideStr.length; i++) {
            let char = sideStr[i];
            if (char === '{') depth++; 
            else if (char === '}') depth = Math.max(0, depth - 1);
            if (char === '+' && depth === 0) { terms.push(currentTerm.trim()); currentTerm = ""; } 
            else { currentTerm += char; }
        }
        terms.push(currentTerm.trim());
        return terms.filter(t => t !== "");
    }

    const leftTerms = splitTerms(leftSide), rightTerms = splitTerms(rightSide);

    function makeExact(terms) {
        return terms.map(t => t.replace(/\s+/g, '').replace(/\\left|\\right|[\{\}]/g, '').replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+').replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-').replace(/_\((s|l|g|aq)\)/gi, '($1)').replace(/_+/g, '_').replace(/^1(?=[a-zA-Z\\])/, '')).sort().join("+");
    }
    function makeSloppy(terms) {
        return terms.map(t => t.replace(/[\{\}]/g, '').replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+').replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-').replace(/^1(?=[a-zA-Z\\])/, '').replace(/\\[a-zA-Z]+/g, '').replace(/[_^{}\(\)\[\]]/g, '').replace(/\s+/g, '').toLowerCase()).sort().join("+");
    }
    function makeSpecies(terms) {
        return terms.map(t => t.replace(/\s+/g, '').replace(/^[0-9]+/, '').replace(/\\left|\\right|[\{\}]/g, '').replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+').replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-').replace(/_\((s|l|g|aq)\)/gi, '($1)').replace(/_+/g, '_')).sort().join("+");
    }

    return {
        exact: makeExact(leftTerms) + theArrow + makeExact(rightTerms),
        sloppy: makeSloppy(leftTerms) + "ARROW" + makeSloppy([theArrow]) + "ARROW" + makeSloppy(rightTerms),
        species: makeSpecies(leftTerms) + theArrow + makeSpecies(rightTerms)
    };
}

let teacherDataArray = tas.map(ta => processChem(ta.replace(/^["']|["']$/g, '').replace(/\\\\/g, '\\')));

// --- RESTORED: UI Generation ---
tas.forEach((ta, index) => {
    const wrapper = document.createElement('div');
    wrapper.style.cssText = "border: 1px solid #ced4da; padding: 15px; border-radius: 4px; background: white; display: flex; flex-direction: column; position: relative;";
    
    if (titles[index]) {
        const label = document.createElement('label');
        label.innerHTML = titles[index];
        wrapper.appendChild(label);
        if (window.MathLive) MathLive.renderMathInElement(label);
    }
    const mfRow = document.createElement('div');
    mfRow.style.cssText = "display: flex; align-items: center; width: 100%;";
    const mf = document.createElement('math-field');
    mf.style.cssText = "width: 100%; font-size: 1.4em; background-color: #ffffff; border: 2px solid #93c5fd; border-radius: 8px; padding: 12px;";
    
    mf.addEventListener('focusin', () => {
        activeMathField = mf;
        const palette = document.getElementById('button-palette');
        if (palette && wrapper.parentNode) wrapper.parentNode.insertBefore(palette, wrapper);
    });

    mfRow.appendChild(mf);
    wrapper.appendChild(mfRow);
    container.appendChild(wrapper);
    mfs.push({ element: mf, row: mfRow, wrapper: wrapper });
});

// --- RESTORED: Preset Buttons (No Commas) ---
texs.forEach((t) => {
    let display = Array.isArray(t) ? t[0] : t;
    let insert = Array.isArray(t) ? (t[1] || t[0]) : t;
    const btn = document.createElement('div');
    btn.style.cssText = "cursor: pointer; width: 85px; height: 45px; border: 1px solid #ced4da; background: #f8f9fa; display: flex; align-items: center; justify-content: center; border-radius: 4px;";
    btn.innerHTML = `\\( ${display} \\)`;
    if (window.MathLive) MathLive.renderMathInElement(btn);
    btn.addEventListener('click', (e) => {
        e.preventDefault();
        if (!activeMathField) return;
        if (insert === "^") activeMathField.executeCommand("moveToSuperscript");
        else if (insert === "_") activeMathField.executeCommand("moveToSubscript");
        else activeMathField.insert(insert);
        activeMathField.focus();
    });
    btnContainer.appendChild(btn);
});

// --- RESTORED: STACK Logic (With ans2 Number Fix) ---
stack_js.request_access_to_input("ans1", true).then((inputId1) => {
    const hiddenInput1 = document.getElementById(inputId1);
    stack_js.request_access_to_input("ans2", true).then((inputId2) => {
        const hiddenInput2 = document.getElementById(inputId2);
        const isLocked = hiddenInput1.readOnly || hiddenInput1.disabled;

        function evaluateStudent() {
            if (isLocked) return;
            let currentAnswers = [], currentGrades = [];
            
            mfs.forEach((mfObj, index) => {
                let rawLatex = mfObj.element.getValue("latex") || "";
                currentAnswers.push(rawLatex.replace(/\\/g, '\\\\'));
                
                let sData = processChem(rawLatex), tData = teacherDataArray[index];
                let grade = (rawLatex.trim() === '') ? 0 : (sData.exact === tData.exact ? 1 : (sData.species === tData.species || sData.sloppy === tData.sloppy ? partialCredit : 0));
                currentGrades.push(grade);
            });
            hiddenInput1.value = JSON.stringify(currentAnswers);
            hiddenInput2.value = JSON.stringify(currentGrades); // CLEAN: Numbers only
            hiddenInput1.dispatchEvent(new Event('change'));
            hiddenInput2.dispatchEvent(new Event('change'));
        }

        let saved = parseStackJSON(inputId1, []);
        mfs.forEach((m, i) => {
            if (saved[i]) m.element.setValue(saved[i].replace(/\\\\/g, '\\'), {suppressChangeNotifications: true});
            m.element.addEventListener('input', evaluateStudent);
        });

        if (isLocked) {
            let finalGrades = parseStackJSON(inputId2, []);
            mfs.forEach((mfObj, index) => {
                mfObj.element.readOnly = true;
                let grade = finalGrades[index];
                let sData = processChem(mfObj.element.getValue("latex")), tData = teacherDataArray[index];

                // --- RESTORED: Premium SVGs ---
                const svgCheck = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#4caf50"><path d="M470.6 105.4c12.5 12.5 12.5 32.8 0 45.3l-256 256c-12.5 12.5-32.8 12.5-45.3 0l-128-128c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0L192 338.7 425.4 105.4c12.5-12.5 32.8-12.5 45.3 0z"/></svg>`;
                const svgAdjust = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#ff9800"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zM256 464c-114.7 0-208-93.3-208-208S141.3 48 256 48v416z"/></svg>`;
                const svgTimes = `<svg viewBox="0 0 320 512" width="24" height="24" fill="#f44336"><path d="M310.6 150.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L160 210.7 54.6 105.4c-12.5-12.5-32.8-12.5-45.3 0s-12.5 32.8 0 45.3L114.7 256 9.4 361.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 301.3 265.4 406.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L205.3 256 310.6 150.6z"/></svg>`;
                
                let markIcon = document.createElement('div');
                markIcon.style.marginLeft = "15px";
                if (grade === 1) markIcon.innerHTML = svgCheck; 
                else if (grade > 0) markIcon.innerHTML = svgAdjust; 
                else markIcon.innerHTML = svgTimes; 
                mfObj.row.appendChild(markIcon);

                if (grade < 1 && mfObj.element.getValue().trim() !== "") {
                    let errDiv = document.createElement('div');
                    errDiv.style.cssText = "color: #d32f2f; font-weight: bold; margin-top: 10px; direction: rtl; text-align: right; background-color: #ffebee; padding: 6px 12px; border-radius: 6px; border: 1px solid #ffcdd2;";
                    if (sData.species === tData.species) errDiv.innerText = errorMessages[0];
                    else if (sData.sloppy === tData.sloppy) errDiv.innerText = errorMessages[1];
                    else errDiv.innerText = errorMessages[2];
                    mfObj.wrapper.appendChild(errDiv);
                }
            });
        } else { evaluateStudent(); }
    });
});
[[/script]]
[[/iframe]]
