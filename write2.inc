[[iframe width="100%" height="450px"]]
[[script src="https://unpkg.com/mathlive"]][[/script]]
[[style]]
    /* This targets BOTH the keyboard icon and the menu icon */
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
        direction: rtl !important; /* <-- Adds absolute RTL enforcement */
    }
[[/style]]

<div style="display: none;">
    <div id="data-tas">{#js_tas#}</div>
    <div id="data-prefixes">{#js_prefixes#}</div>
    <div id="data-titles">{#js_titles#}</div>
    <div id="data-tex">{#js_tex#}</div>
    <div id="data-partial">{#js_partial_credit#}</div>
    <div id="data-debug">{#debug_display#}</div> 
</div>

<div style="font-family: sans-serif; padding: 10px; box-sizing: border-box;">
    <div id="button-palette" style="background: white; border: 1px solid #ced4da; border-radius: 6px; margin-bottom: 5px; overflow: hidden; width: fit-content; max-width: 100%; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
        <div id="palette-title" dir="rtl" style="padding: 8px 12px; background: #e9ecef; border-bottom: 1px solid #ced4da; font-weight: bold; font-size: 0.85em; color: #495057;">
          Symbols & Templates
        </div>
        <div id="preset-buttons-container" style="display: flex; flex-wrap: wrap; gap: 8px; padding: 10px;"></div>
    </div>
    
    <div id="mathfields-container" style="display: flex; flex-direction: column; gap: 15px;">
    </div>
</div>

[[script type="module"]]
import {stack_js} from '[[cors src="stackjsiframe.js"/]]';
try {
    let customTitle = {#js_title#};
    if (customTitle) document.getElementById('palette-title').innerText = customTitle;
} catch (e) { console.log("No custom title provided."); }

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
    
/*    function splitTerms(sideStr) {
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
*/
function splitTerms(sideStr) {
        let terms = [], currentTerm = "", depth = 0;
      // --- THE FIXED VERSION ---
for (let i = 0; i < sideStr.length; i++) {
    let char = sideStr[i];

    // We only track curly braces {} because they are the only 
    // characters LaTeX uses to "hide" math like charges.
    if (char === '{') {
        depth++;
    } else if (char === '}') {
        // The Medicine: prevents negative depth from rogue arrow brackets
        depth = Math.max(0, depth - 1);
    }
    
    // Parentheses () and Brackets [] are now IGNORED. 
    // This prevents (aq) from dropping the depth back to 0 too early.

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
/*
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
*/
function makeExact(terms) {
        return terms.map(t => {
            return t.replace(/\s+/g, '')
                    .replace(/\\left/g, '')
                    .replace(/\\right/g, '')
                    .replace(/[\{\}]/g, '')
                    
                    // EXPLICIT POSITIVE LOGIC
                    .replace(/\^\+(\d+)/g, '^$1+') 
                    .replace(/\^1\+/g, '^+')       
                    
                    // EXPLICIT NEGATIVE LOGIC
                    .replace(/\^\-(\d+)/g, '^$1-') 
                    .replace(/\^1\-/g, '^-')       
                    
                    .replace(/_\((s|l|g|aq)\)/gi, '($1)')
                    .replace(/_+/g, '_');
        }).sort().join("+");
    }

    function makeSloppy(terms) {
        return terms.map(t => {
            return t.replace(/[\{\}]/g, '')
            
                    // EXPLICIT POSITIVE & NEGATIVE LOGIC
                    .replace(/\^\+(\d+)/g, '^$1+')
                    .replace(/\^1\+/g, '^+')
                    .replace(/\^\-(\d+)/g, '^$1-')
                    .replace(/\^1\-/g, '^-')
                    
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
        label.style.cssText = "font-weight: bold; color: #212529; margin-bottom: 8px; display: block; direction: rtl; text-align: right;";
        label.innerHTML = titles[index];
        wrapper.appendChild(label);
        
        // Tell MathLive to render the \( \) or $$ $$ math
        if (window.MathLive) {
            MathLive.renderMathInElement(label);
        }
    }
        // Create Flex container for Mathfield + Icon
    const mfRow = document.createElement('div');
    mfRow.style.cssText = "display: flex; align-items: center; width: 100%;";
    
    // Create Mathfield
    const mf = document.createElement('math-field');
    // --- ADD THESE TWO LINES TO KILL THE ICONS FOR GOOD ---
    mf.menuToggleVisibility = "hidden";
    mf.virtualKeyboardToggleVisibility = "hidden";
    // ------------------------------------------------------
    mf.style.cssText = "width: 100%; font-size: 1.4em; background-color: #ffffff; border: 2px solid #93c5fd; border-radius: 8px; padding: 12px; box-sizing: border-box; box-shadow: 0 2px 4px rgba(0,0,0,0.05); transition: all 0.2s ease;";    
    
    // Update tracker on focus and move the toolbar ABOVE the entire question box
    mf.addEventListener('focusin', () => {
        activeMathField = mf;
        
        // Add a soft blue glowing ring to the active box (Matches #93c5fd)
        mf.style.boxShadow = "0 0 0 4px rgba(147, 197, 253, 0.4)";
        
        const palette = document.getElementById('button-palette');

        if (palette && wrapper.parentNode) {
            wrapper.parentNode.insertBefore(palette, wrapper);
        }
    });

    // Remove glow when leaving
    mf.addEventListener('focusout', () => {
        mf.style.boxShadow = "0 2px 4px rgba(0,0,0,0.05)";
    });

  
    // ---------------------------------

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

   // --- Add these hover effects right after your btn.style.cssText ---
    btn.addEventListener('mouseenter', () => {
        btn.style.backgroundColor = '#e2e6ea'; // Slightly darker gray
        btn.style.boxShadow = '0 2px 4px rgba(0,0,0,0.1)'; // Optional subtle shadow
    });
    
    btn.addEventListener('mouseleave', () => {
        btn.style.backgroundColor = '#f8f9fa'; // Back to original color
        btn.style.boxShadow = 'none'; // Remove shadow
    });

   // Use a standard text span with LaTeX delimiters instead of a heavy math-field
    const mathHtml = `<span style="font-size: 1.2em; color: black; pointer-events: none;">\\( ${displayTex} \\)</span>`;
    const overlayHtml = `<div style="position: absolute; top: 0; left: 0; right: 0; bottom: 0; z-index: 100;"></div>`;
    
    btn.innerHTML = mathHtml + overlayHtml;
    
    // Tell MathLive to render this span as static, non-editable math
    if (window.MathLive) {
        MathLive.renderMathInElement(btn);
    }
    
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

        // --- 1. Load the boxes normally & Set up Listeners ---
        mfs.forEach((mfObj, index) => {
            // Load saved data or prefixes
            if (savedAnswers[index]) {
                mfObj.element.setValue(savedAnswers[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            } else if (prefixes[index]) {
                mfObj.element.setValue(prefixes[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            }

           // ONE Listener: Saves to STACK AND updates Teacher Box (WITH DEBOUNCE)
            let debounceTimer;
            mfObj.element.addEventListener('input', () => {
                clearTimeout(debounceTimer);
                debounceTimer = setTimeout(() => {
                    evaluateStudent();   
                    updateTeacherBox();  
                }, 400); // Waits 400ms after the student stops typing
            });
        });

        // Sync everything immediately on page load
        if (!isLocked) {
            evaluateStudent();
            updateTeacherBox();
        }

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
    }); // End of ans2 promise
}); // End of ans1 promise

// --- 7. Teacher Helper Function (Outside the promise to keep it clean) ---
function updateTeacherBox() {
    const teacherBox = window.parent.document.getElementById('teacher-box');
    if (!teacherBox) return;

    let currentAnswers = mfs.map(mf => mf.element.getValue());
    let maximaArray = '["' + currentAnswers.join('", "') + '"]';
    maximaArray = maximaArray.replace(/\\/g, '\\\\');
    const newText = "👨‍🏫 Teacher Maxima Code (Copy & Paste):\n\n" + "ta: " + maximaArray + ";";

    if (teacherBox.innerText !== newText) {
        teacherBox.innerText = newText;
    }
}
// --- 8. Auto-Resize iFrame ---
function autoResize() {
    try {
        // FRAME_ID is a hidden variable automatically created by STACK
        const parentIframe = window.parent.document.getElementById(FRAME_ID);
        // STACK also wraps the iframe in a holder div we need to expand
        const parentHolder = window.parent.document.getElementById(FRAME_ID.replace('stack-iframe', 'stack-iframe-holder'));
        
        if (parentIframe && parentHolder) {
            // Calculate the total height of the content plus a little extra padding
            const newHeight = (document.body.scrollHeight + 20) + 'px';
            
            parentIframe.style.height = newHeight;
            parentHolder.style.height = newHeight;
        }
    } catch (e) {
        console.warn("Could not auto-resize iframe:", e);
    }
}

// Run once immediately
autoResize();

// Keep watching the body for any size changes (like wrapping text or opening menus)
const resizeObserver = new ResizeObserver(() => {
    autoResize();
});
resizeObserver.observe(document.body);
[[/script]]
[[/iframe]]
