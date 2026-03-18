<div><br class="Apple-interchange-newline">[[iframe width="100%" height="450px"]]<br><br>[[script src="https://unpkg.com/mathlive"]][[/script]]<br>[[style]]
    /* This targets BOTH the keyboard icon and the menu icon */
    math-field::part(virtual-keyboard-toggle),
    math-field::part(menu-toggle) {
      display: none !important;
    }
[[/style]]

<div style="display: none;">
    <div id="data-tas">{#js_tas#}</div>
    <div id="data-prefixes">{#js_prefixes#}</div>
    <div id="data-titles">{#js_titles#}</div>
    <div id="data-tex">{#js_tex#}</div>
    <div id="data-title">{#js_title#}</div>
    <div id="data-partial">{#js_partial_credit#}</div>
    <div id="data-debug">{#debug_display#}</div> 
</div>

<div style="font-family: sans-serif; padding: 10px; box-sizing: border-box;">
    <div id="button-palette" style="background: white; border: 1px solid #ced4da; border-radius: 6px; margin-bottom: 25px; overflow: hidden; width: fit-content; max-width: 100%;">
        <div id="palette-title" style="padding: 8px 12px; background: #e9ecef; border-bottom: 1px solid #ced4da; font-weight: bold; font-size: 0.85em; color: #495057;">
            Symbols & Templates
        </div>
        <div id="preset-buttons-container" style="display: flex; flex-wrap: wrap; gap: 8px; padding: 10px;"></div>
    </div>
    
    <div id="mathfields-container" style="display: flex; flex-direction: column; gap: 15px;">
        </div>
</div>

[[script type="module"]]
import {stack_js} from '[[cors src="stackjsiframe.js"/]]';

// --- 1. Parse Maxima Variables Safely ---
function getStackData(id) {
    let el = document.getElementById(id);
    if (!el) return null;
    let txt = el.innerText || el.textContent;
    if (txt.includes('{#')) return null; 
    return txt.trim();
}

try {
    let customTitle = getStackData('data-title');
    if (customTitle) {
        // Remove extra quotes if STACK added them
        customTitle = customTitle.replace(/^"|"$/g, ''); 
        document.getElementById('palette-title').innerText = customTitle;
    }
} catch (e) {}

let tas = [];
let prefixes = [];
let titles = [];
let texs = [];
let partialCredit = 0.5;

// Safely unpack data, handling STACK's extra quotes
function parseStackJSON(id, fallback) {
    try {
        let rawData = getStackData(id);
        if (!rawData) return fallback;
        
        let parsed = JSON.parse(rawData);
        // If STACK double-quoted it, it will parse as a string. Parse it again!
        if (typeof parsed === 'string') {
            parsed = JSON.parse(parsed);
        }
        
        // Force it to be an array just to be perfectly safe
        if (!Array.isArray(parsed)) parsed = [parsed];
        return parsed;
        
    } catch (e) {
        console.warn("Could not parse " + id, e);
        return fallback;
    }
}

tas = parseStackJSON('data-tas', []);
prefixes = parseStackJSON('data-prefixes', []);
titles = parseStackJSON('data-titles', []);
texs = parseStackJSON('data-tex', []);

try {
    let parsedPartial = parseFloat(getStackData('data-partial'));
    if (!isNaN(parsedPartial)) partialCredit = parsedPartial;
} catch (e) {}

// --- 2. Initialize Shared Tracking Variables ---
const btnContainer = document.getElementById('preset-buttons-container');
const container = document.getElementById('mathfields-container');
let mfs = []; // Array to hold references to all generated mathfields
let activeMathField = null; // The Tracker

// --- 3. Process Teacher Answers ---
function processChem(latex) {
    latex = latex.replace(/\\textcolor\{([^}]+)\}/g, 'COLOR$1');
    let arrowRegex = /((?:\\longrightarrow|\\xrightarrow|\\rightarrow|\\rightleftharpoons|\\Rightarrow)(?:_\{.*?\}|\^\{.*?\}|\{.*?\}|\[.*?\])*|=)/;
    let parts = latex.split(arrowRegex);
    
    if (parts.length !== 3) {
        let exact = latex.replace(/\s+/g, '');
        let sloppy = latex.replace(/\\[a-zA-Z]+/g, '').replace(/[_^{}\(\)\[\]]/g, '').replace(/\s+/g, '').toLowerCase();
        return { exact, sloppy };
    }
    
    let leftSide = parts[0];
    let rightSide = parts[2];
    let theArrow = parts[1].replace(/\s+/g, '').replace(/\{\}/g, ''); 
    
    function splitTerms(sideStr) {
        let terms = [], currentTerm = "", depth = 0;
        for (let i = 0; i < sideStr.length; i++) {
            let char = sideStr[i];
            if (char === '{' || char === '(' || char === '[') depth++;
            else if (char === '}' || char === ')' || char === ']') depth--;
            
            if (char === '+' && depth === 0) {
                terms.push(currentTerm.trim());
                currentTerm = "";
            } else {
                currentTerm += char;
            }
        }
        terms.push(currentTerm.trim());
        return terms.filter(t => t !== "");
    }

    let leftTerms = splitTerms(leftSide);
    let rightTerms = splitTerms(rightSide);

    function makeExact(terms) {
        return terms.map(t => {
            return t.replace(/\s+/g, '')
                    .replace(/\\left/g, '')
                    .replace(/\\right/g, '')
                    .replace(/[\{\}]/g, '')
                    .replace(/\^([+-])(\d+)/g, '^$2$1')
                    .replace(/\^1([+-])/g, '^$1')
                    .replace(/_\((s|l|g|aq)\)/gi, '($1)')
                    .replace(/_+/g, '_');
        }).sort().join("+");
    }

    function makeSloppy(terms) {
        return terms.map(t => {
            return t.replace(/[\{\}]/g, '')
                    .replace(/\^([+-])(\d+)/g, '^$2$1')
                    .replace(/\^1([+-])/g, '^$1')
                    .replace(/\\[a-zA-Z]+/g, '')
                    .replace(/[_^{}\(\)\[\]]/g, '')
                    .replace(/\s+/g, '')
                    .toLowerCase();
        }).sort().join("+");
    }

    let exactStr = makeExact(leftTerms) + theArrow + makeExact(rightTerms);
    let sloppyArrow = makeSloppy([theArrow]); 
    let sloppyStr = makeSloppy(leftTerms) + "ARROW" + sloppyArrow + "ARROW" + makeSloppy(rightTerms);

    return { exact: exactStr, sloppy: sloppyStr };
}

// Pre-calculate teacher data for all inputs
let teacherDataArray = tas.map(ta => {
    let cleanTa = ta.replace(/^["']|["']$/g, '').replace(/\\\\/g, '\\');
    return processChem(cleanTa);
});

// --- 4. Build Dynamic Inputs ---
tas.forEach((ta, index) => {
    const wrapper = document.createElement('div');
    wrapper.style.cssText = "border: 1px solid #ced4da; padding: 15px; border-radius: 4px; background: white; display: flex; flex-direction: column; position: relative;";
    
    // Add title if exists
    if (titles[index]) {
        const label = document.createElement('label');
        label.style.cssText = "font-weight: bold; color: #4338ca; margin-bottom: 8px; display: block;";
        label.innerHTML = titles[index];
        wrapper.appendChild(label);
    }
    
    // Create Flex container for Mathfield + Icon
    const mfRow = document.createElement('div');
    mfRow.style.cssText = "display: flex; align-items: center; width: 100%;";
    
    // Create Mathfield
    const mf = document.createElement('math-field');
    mf.style.cssText = "width: 100%; font-size: 1.4em;";
    
    // Update tracker on focus
    mf.addEventListener('focusin', () => {
        activeMathField = mf;
    });
    
    mfRow.appendChild(mf);
    wrapper.appendChild(mfRow);
    container.appendChild(wrapper);
    mfs.push({ element: mf, row: mfRow });
});

// Default active to the first field
if (mfs.length > 0) { activeMathField = mfs[0].element; }

// --- 5. Build Preset Buttons ---
for (let i = 0; i < texs.length; i++) {
    let displayTex = Array.isArray(texs[i]) ? texs[i][0] : texs[i];
    let insertTex = Array.isArray(texs[i]) ? (texs[i][1] || texs[i][0]) : texs[i];
    
    const btn = document.createElement('div');
    btn.style.cssText = "position: relative; cursor: pointer; width: 85px; height: 45px; box-sizing: border-box; border: 1px solid #ced4da; border-radius: 4px; background-color: #f8f9fa; display: flex; align-items: center; justify-content: center; user-select: none; transition: all 0.2s;";
    
    const mathHtml = `<math-field tabindex="-1" style="pointer-events: none; background: transparent; border: none; padding: 0; margin: 0; min-height: auto; font-size: 1.1em; --text-color: #000000; --placeholder-color: #000000; --placeholder-opacity: 1; color: #000000;">${displayTex}</math-field>`;
    const overlayHtml = `<div style="position: absolute; top: 0; left: 0; right: 0; bottom: 0; z-index: 100;"></div>`;
    
    btn.innerHTML = mathHtml + overlayHtml;
    
    btn.addEventListener('click', (e) => {
        e.preventDefault(); e.stopPropagation();
        if (!activeMathField) return; 
        
        if (insertTex === "^") {
            activeMathField.executeCommand("moveToSuperscript");
        } else if (insertTex === "_") {
            activeMathField.executeCommand("moveToSubscript");
        } else {
            activeMathField.insert(insertTex); 
        }
        activeMathField.focus(); 
    });
    btnContainer.appendChild(btn);
}

// --- 6. Connect to STACK ---
stack_js.request_access_to_input("ans1", true).then((inputId1) => {
    const hiddenInput1 = document.getElementById(inputId1);
    
    stack_js.request_access_to_input("ans2", true).then((inputId2) => {
        const hiddenInput2 = document.getElementById(inputId2);
        
        const isLocked = hiddenInput1.readOnly || hiddenInput1.disabled || 
                         hiddenInput1.hasAttribute('readonly') || hiddenInput1.hasAttribute('disabled');
        
        function evaluateStudent() {
            if (isLocked) return; 
            
            let currentAnswers = [];
            let currentGrades = [];
            
            mfs.forEach((mfObj, index) => {
                let rawLatex = mfObj.element.getValue("latex");
                currentAnswers.push(rawLatex.replace(/\\/g, '\\\\'));
                
                let studentData = processChem(rawLatex);
                let tData = teacherDataArray[index];
                
                if (studentData.exact === tData.exact) {
                    currentGrades.push(1);
                } else if (studentData.sloppy === tData.sloppy) {
                    currentGrades.push(partialCredit);
                } else {
                    currentGrades.push(0);
                }
            });
            
            let newAnsStr = JSON.stringify(currentAnswers);
            let newGradeStr = JSON.stringify(currentGrades);
            
            if (hiddenInput1.value !== newAnsStr) {
                hiddenInput1.value = newAnsStr;
                hiddenInput1.dispatchEvent(new Event('change'));
            }
            if (hiddenInput2.value !== newGradeStr) {
                hiddenInput2.value = newGradeStr;
                hiddenInput2.dispatchEvent(new Event('change'));
            }
        }

      let savedAnswers = [];
        if (hiddenInput1.value && hiddenInput1.value.trim() !== "") {
            try { 
                let parsed = JSON.parse(hiddenInput1.value);
                // If Moodle double-quoted the correct answer, unpack it again!
                if (typeof parsed === 'string') parsed = JSON.parse(parsed);
                if (Array.isArray(parsed)) savedAnswers = parsed;
            } catch(e) {}
        }
     // --- 1. Load the boxes normally ---
        mfs.forEach((mfObj, index) => {
            if (savedAnswers[index]) {
                mfObj.element.setValue(savedAnswers[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            } else if (prefixes[index]) {
                mfObj.element.setValue(prefixes[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            }
            mfObj.element.addEventListener('input', evaluateStudent);
        });

        if (!isLocked) evaluateStudent();


        // --- VISUAL MARKING (If Submitted) ---
        if (isLocked) {
            btnContainer.style.pointerEvents = "none"; 
            btnContainer.style.opacity = "0.6"; 
            
            let finalGrades = [];
            try { finalGrades = JSON.parse(hiddenInput2.value); } catch(e) {}
            
            mfs.forEach((mfObj, index) => {
                mfObj.element.readOnly = true; 
                let grade = finalGrades[index];
                
                if (grade !== undefined && !isNaN(grade)) {
                    const markIcon = document.createElement('div');
                    markIcon.style.cssText = "margin-left: 15px; display: flex; align-items: center;";
                    
                    const svgCheck = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#4caf50"><path d="M470.6 105.4c12.5 12.5 12.5 32.8 0 45.3l-256 256c-12.5 12.5-32.8 12.5-45.3 0l-128-128c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0L192 338.7 425.4 105.4c12.5-12.5 32.8-12.5 45.3 0z"/></svg>`;
                    const svgAdjust = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#ff9800"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zM256 464c-114.7 0-208-93.3-208-208S141.3 48 256 48v416z"/></svg>`;
                    const svgTimes = `<svg viewBox="0 0 320 512" width="24" height="24" fill="#f44336"><path d="M310.6 150.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L160 210.7 54.6 105.4c-12.5-12.5-32.8-12.5-45.3 0s-12.5 32.8 0 45.3L114.7 256 9.4 361.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 301.3 265.4 406.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L205.3 256 310.6 150.6z"/></svg>`;
                    
                    if (grade === 1) markIcon.innerHTML = svgCheck; 
                    else if (grade > 0 && grade < 1) markIcon.innerHTML = svgAdjust; 
                    else if (grade === 0) markIcon.innerHTML = svgTimes; 
                    
                    mfObj.row.appendChild(markIcon);
                }
            });
        }
// --- 7. Teacher Helper: Clean Copy Box (Cross-Boundary Copy) ---
function updateTeacherBox() {
    const parentDoc = window.parent.document;
    const teacherBox = parentDoc.getElementById('teacher-box');
    
    if (!teacherBox) return;

    // 1. Collect and format the values
    let currentAnswers = mfs.map(mf => mf.element.getValue());
    let maximaArray = '["' + currentAnswers.join('", "') + '"]';
    maximaArray = maximaArray.replace(/\\/g, '\\\\');
    const finalCode = "ta: " + maximaArray + ";";

    // 2. Inject the formatted text
    teacherBox.innerText = finalCode;
}

// Sync it up
mfs.forEach(mfObj => mfObj.element.addEventListener('input', updateTeacherBox));
setInterval(updateTeacherBox, 500);
    });
});
[[/script]]
[[/iframe]]</div>
