[[iframe width="100%" height="450px"]]
[[script src="https://unpkg.com/mathlive"]][[/script]]
[[style]]
    /* Hide the default MathLive virtual keyboard and menu toggles */
    math-field::part(virtual-keyboard-toggle),
    math-field::part(menu-toggle) {
        display: none !important;
    }
    /* Style the labels above the math-fields, configured for RTL (Hebrew) */
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
[[/style]]<div style="display: none;">
    <div id="data-tas">{#js_tas#}</div>
    <div id="data-prefixes">{#js_prefixes#}</div>
    <div id="data-titles">{#js_titles#}</div>
    <div id="data-tex">{#js_tex#}</div>
    <div id="data-partial">{#js_partial_credit#}</div>
    <div id="data-debug">{#debug_display#}</div> 
    <div id="data-errors" style="display: none;">{#js_error_dict#}</div></div><div style="font-family: sans-serif; padding: 10px; box-sizing: border-box;">
        <div id="button-palette" style="background: white; border: 1px solid #ced4da; border-radius: 6px; margin-bottom: 5px; overflow: hidden; width: fit-content; max-width: 100%; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
        <div id="palette-title" dir="rtl" style="padding: 8px 12px; background: #e9ecef; border-bottom: 1px solid #ced4da; font-weight: bold; font-size: 0.85em; color: #495057;">
          Symbols & Templates
        </div>
                <div id="preset-buttons-container" style="display: flex; flex-wrap: wrap; gap: 8px; padding: 10px;"></div>
    </div>
    
        <div id="mathfields-container" style="display: flex; flex-direction: column; gap: 15px;">
    </div></div>

[[script type="module"]]
// Forcefully inject CSS to hide MathLive icons
const hideIconsStyle = document.createElement('style');
hideIconsStyle.innerHTML = `
    math-field::part(virtual-keyboard-toggle),
    math-field::part(menu-toggle) {
        display: none !important;
    }
`;
// Attach it to the head of the document where it applies universally
document.head.appendChild(hideIconsStyle);
// Import STACK's Javascript API for communication
import {stack_js} from '[[cors src="stackjsiframe.js"/]]';
// Attempt 1: Set a custom title from the direct STACK tag
try {
    let customTitle = {#js_title#};
    if (customTitle) document.getElementById('palette-title').innerText = customTitle;
} catch (e) { console.log("No custom title provided."); }

// Helper function to safely extract text from hidden DOM bridges
function getStackData(id) {
    let el = document.getElementById(id);
    if (!el) return null;
    let txt = el.innerText || el.textContent;
    if (txt.includes('{#')) return null; 
    return txt.trim();
}

// Attempt 2: Fallback to set a custom title from a hidden div
try {
    let customTitle = getStackData('data-title');
    if (customTitle) {
        customTitle = customTitle.replace(/^"|"$/g, ''); 
        document.getElementById('palette-title').innerText = customTitle;
    }
} catch (e) {}

// Initialize global state arrays
let tas = [];
let prefixes = [];
let titles = [];
let texs = [];
let partialCredit = 0.5;

// Helper function to parse JSON data pulled from hidden divs
function parseStackJSON(id, fallback) {
    try {
        let rawData = getStackData(id);
        if (!rawData) return fallback;
        
        let parsed = JSON.parse(rawData);
        if (typeof parsed === 'string') {
            parsed = JSON.parse(parsed);
        }
        
        if (!Array.isArray(parsed)) parsed = [parsed];
        return parsed;
        
    } catch (e) {
        return fallback;
    }
}

// Populate the state arrays using the STACK data
tas = parseStackJSON('data-tas', []);
prefixes = parseStackJSON('data-prefixes', []);
titles = parseStackJSON('data-titles', []);
texs = parseStackJSON('data-tex', []);

// Override default partial credit if STACK provides a specific value
try {
    let parsedPartial = parseFloat(getStackData('data-partial'));
    if (!isNaN(parsedPartial)) partialCredit = parsedPartial;
} catch (e) {}

// Load error dictionary from STACK
let errorMessages = parseStackJSON('data-errors', []);

// Reference variables for dynamic UI construction
const btnContainer = document.getElementById('preset-buttons-container');
const container = document.getElementById('mathfields-container');
let mfs = []; 
let activeMathField = null; 

// Chemistry Engine: Standardizes LaTeX input into multiple strictness levels for grading
function processChem(latex) {
    latex = latex.replace(/\\textcolor\{([^}]+)\}/g, 'COLOR$1');
    let arrowRegex = /((?:\\longrightarrow|\\xrightarrow|\\rightarrow|\\rightleftharpoons|\\Rightarrow)(?:_\{.*?\}|\^\{.*?\}|\{.*?\}|\[.*?\])*|=)/;
    let parts = latex.split(arrowRegex);
    
    // If no reaction arrow is found, just standardize the single string
    if (parts.length !== 3) {
        let exact = latex.replace(/\s+/g, '');
        let sloppy = latex.replace(/\\[a-zA-Z]+/g, '').replace(/[_^{}\(\)\[\]]/g, '').replace(/\s+/g, '').toLowerCase();
        return { exact, sloppy, species: sloppy, superSloppy: sloppy }; 
    }
    
    let leftSide = parts[0];
    let rightSide = parts[2];
    let theArrow = parts[1].replace(/\s+/g, '').replace(/\{\}/g, ''); 
    
    // Splits sides of a reaction safely into individual chemical terms
    function splitTerms(sideStr) {
        let terms = [], currentTerm = "", depth = 0;
        for (let i = 0; i < sideStr.length; i++) {
            let char = sideStr[i];
            if (char === '{') { depth++; } 
            else if (char === '}') { depth = Math.max(0, depth - 1); }
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

    // Creates an exact comparison string, standardizing charges and brackets
    function makeExact(terms) {
        return terms.map(t => {
            return t.replace(/\s+/g, '')
                    .replace(/\\left/g, '').replace(/\\right/g, '').replace(/[\{\}]/g, '')
                    .replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+')       
                    .replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-')       
                    .replace(/_\((s|l|g|aq)\)/gi, '($1)').replace(/_+/g, '_')
                    .replace(/^1(?=[a-zA-Z\\])/, ''); 
        }).sort().join("+");
    }

    // Creates a sloppy comparison string ignoring formatting characters
    function makeSloppy(terms) {
        return terms.map(t => {
            return t.replace(/[\{\}]/g, '')
                    .replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+')
                    .replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-')
                    .replace(/^1(?=[a-zA-Z\\])/, '').replace(/\\[a-zA-Z]+/g, '')
                    .replace(/[_^{}\(\)\[\]]/g, '').replace(/\s+/g, '').toLowerCase();
        }).sort().join("+");
    }

    // Strips leading coefficients entirely to check if correct molecules are present
    function makeSpecies(terms) {
        return terms.map(t => {
            let stripped = t.replace(/\s+/g, '').replace(/^[0-9]+/, ''); 
            return stripped.replace(/\\left/g, '').replace(/\\right/g, '').replace(/[\{\}]/g, '')
                    .replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+')       
                    .replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-')       
                    .replace(/_\((s|l|g|aq)\)/gi, '($1)').replace(/_+/g, '_');
        }).sort().join("+");
    }

    // BRAND NEW: Strips BOTH coefficients and formatting to check if core atoms are right
    function makeSuperSloppy(terms) {
        return terms.map(t => {
            let stripped = t.replace(/\s+/g, '').replace(/^[0-9]+/, ''); 
            return stripped.replace(/[\{\}]/g, '')
                    .replace(/\^\+(\d+)/g, '^$1+').replace(/\^1\+/g, '^+')
                    .replace(/\^\-(\d+)/g, '^$1-').replace(/\^1\-/g, '^-')
                    .replace(/\\[a-zA-Z]+/g, '')
                    .replace(/[_^{}\(\)\[\]]/g, '').toLowerCase();
        }).sort().join("+");
    }

    // Generates the final comparison strings
    let exactStr = makeExact(leftTerms) + theArrow + makeExact(rightTerms);
    
    let sloppyArrow = makeSloppy([theArrow]); 
    let sloppyStr = makeSloppy(leftTerms) + "ARROW" + sloppyArrow + "ARROW" + makeSloppy(rightTerms);
    
    let speciesStr = makeSpecies(leftTerms) + theArrow + makeSpecies(rightTerms);

    let superSloppyArrow = makeSuperSloppy([theArrow]); 
    let superSloppyStr = makeSuperSloppy(leftTerms) + "ARROW" + superSloppyArrow + "ARROW" + makeSuperSloppy(rightTerms);

    return { exact: exactStr, sloppy: sloppyStr, species: speciesStr, superSloppy: superSloppyStr };
}

// Pre-process all teacher answers through the chemistry engine
let teacherDataArray = tas.map(ta => {
    let cleanTa = ta.replace(/^["']|["']$/g, '').replace(/\\\\/g, '\\');
    return processChem(cleanTa);
});

// Builds the math-fields based on the number of Teacher Answers
tas.forEach((ta, index) => {
    const wrapper = document.createElement('div');
    wrapper.setAttribute('dir', 'rtl');
    wrapper.style.cssText = "border: 1px solid #ced4da; padding: 15px; border-radius: 4px; background: white; display: flex; flex-direction: column; position: relative;";
    
    // Add corresponding title above the math-field
    if (titles[index]) {
        const label = document.createElement('label');
        label.setAttribute('dir', 'rtl');
        label.style.cssText = "background-color: #f4f8fa; color: #0073a5; padding: 6px 16px; border-radius: 20px; border: 1px solid #b8daff; display: inline-block; width: fit-content; margin-bottom: 12px; margin-left: auto; margin-right: 0; box-shadow: 0 2px 4px rgba(0,0,0,0.05); font-size: 1.1em; text-align: right; direction: rtl; font-weight: bold;";
        label.innerHTML = titles[index];
        wrapper.appendChild(label);
        if (window.MathLive) { MathLive.renderMathInElement(label); }
    }
    const mfRow = document.createElement('div');
    mfRow.style.cssText = "display: flex; align-items: center; width: 100%;";
    
    const mf = document.createElement('math-field');
        mf.setAttribute('dir', 'ltr');
        mf.style.cssText = "width: 100%; font-size: 1.4em; background-color: #ffffff; border: 2px solid #93c5fd; border-radius: 8px; padding: 12px; box-sizing: border-box; box-shadow: 0 2px 4px rgba(0,0,0,0.05); transition: all 0.2s ease;";    
    
    // Move the symbol palette to float above the currently focused field
    mf.addEventListener('focusin', () => {
        activeMathField = mf;
        mf.style.boxShadow = "0 0 0 4px rgba(147, 197, 253, 0.4)";
        const palette = document.getElementById('button-palette');
        if (palette && wrapper.parentNode) { wrapper.parentNode.insertBefore(palette, wrapper); }
    });

    mf.addEventListener('focusout', () => { mf.style.boxShadow = "0 2px 4px rgba(0,0,0,0.05)"; });

    mfRow.appendChild(mf);
    wrapper.appendChild(mfRow);
    container.appendChild(wrapper);
    mfs.push({ element: mf, row: mfRow });
    
});

if (mfs.length > 0) { activeMathField = mfs[0].element; }

// Builds custom palette buttons based on the STACK `data-tex` variable
for (let i = 0; i < texs.length; i++) {
    let displayTex = Array.isArray(texs[i]) ? texs[i][0] : texs[i];
    let insertTex = Array.isArray(texs[i]) ? (texs[i][1] || texs[i][0]) : texs[i];
    
    const btn = document.createElement('div');
    btn.style.cssText = "position: relative; cursor: pointer; width: 85px; height: 45px; box-sizing: border-box; border: 1px solid #ced4da; border-radius: 4px; background-color: #f8f9fa; display: flex; align-items: center; justify-content: center; user-select: none; transition: all 0.2s;";

    btn.addEventListener('mouseenter', () => {
        btn.style.backgroundColor = '#e2e6ea'; 
        btn.style.boxShadow = '0 2px 4px rgba(0,0,0,0.1)'; 
    });
    btn.addEventListener('mouseleave', () => {
        btn.style.backgroundColor = '#f8f9fa'; 
        btn.style.boxShadow = 'none'; 
    });

    const mathHtml = `<span style="font-size: 1.2em; color: black; pointer-events: none;">\\( ${displayTex} \\)</span>`;
    const overlayHtml = `<div style="position: absolute; top: 0; left: 0; right: 0; bottom: 0; z-index: 100;"></div>`;
    btn.innerHTML = mathHtml + overlayHtml;
    if (window.MathLive) { MathLive.renderMathInElement(btn); }
    
    // Button click logic for handling movement commands or text insertion
    btn.addEventListener('click', (e) => {
        e.preventDefault(); e.stopPropagation();
        if (!activeMathField) return; 
        
        if (insertTex === "^") { activeMathField.executeCommand("moveToSuperscript"); } 
        else if (insertTex === "_") { activeMathField.executeCommand("moveToSubscript"); } 
        else { activeMathField.insert(insertTex); }
        activeMathField.focus(); 
    });
    btnContainer.appendChild(btn);
}

// Request STACK input targets for submitting student data
stack_js.request_access_to_input("ans1", true).then((inputId1) => {
    const hiddenInput1 = document.getElementById(inputId1);
    
    stack_js.request_access_to_input("ans2", true).then((inputId2) => {
        const hiddenInput2 = document.getElementById(inputId2);
        
        // Check if the student has already submitted
        const isLocked = hiddenInput1.readOnly || hiddenInput1.disabled || 
                         hiddenInput1.hasAttribute('readonly') || hiddenInput1.hasAttribute('disabled');
        
        // Local evaluation runs on keystrokes to prep grades and errors for STACK submission
        function evaluateStudent() {
            if (isLocked) return; 
            
            let currentAnswers = [];
            let currentGrades = [];
            let currentErrors = [];
            
            mfs.forEach((mfObj, index) => {
                let rawLatex = mfObj.element.getValue("latex") || "";
                currentAnswers.push(rawLatex.replace(/\\/g, '\\\\'));
                
                // Ensure an empty input strictly returns 0
                if (rawLatex.trim() === '') {
                    currentGrades.push(0);
                    currentErrors.push("");
                    return;
                }
                
                let studentData = processChem(rawLatex);
                let tData = teacherDataArray[index];
                
                let grade = 0;
                let currentInputErrors = []; // Array to hold MULTIPLE errors for one box
                
                // Core grading logic comparing student input to teacher answers
                if (studentData.exact === tData.exact) {
                    grade = 1;
                } else {
                    let isUnbalanced = false;
                    let isBadFormat = false;

                    // Figure out exactly what is wrong
                  

// Figure out exactly what is wrong
let teacherHasArrow = tData.exact.match(/\\longrightarrow|\\xrightarrow|\\rightarrow|\\rightleftharpoons|\\Rightarrow|=/);

if (studentData.species === tData.species && teacherHasArrow) {
    // It's a full reaction, but the coefficients are wrong
    isUnbalanced = true;
    grade = partialCredit;
} else if (studentData.sloppy === tData.sloppy) {
    // It's just a single compound (or reaction) with missing subscripts/formatting
    isBadFormat = true;
    grade = partialCredit;
} else if (studentData.superSloppy === tData.superSloppy) {
    // BOTH are wrong! Atoms match, but coeff/format are completely broken.
    if (teacherHasArrow) isUnbalanced = true;
    isBadFormat = true;
    grade = 0; 
} else {
    // Not even the atoms match
    currentInputErrors.push(errorMessages[2] || "הנוסחה לא נכונה");
}
                

                    // Add the errors to the stack
                    if (isUnbalanced) currentInputErrors.push(errorMessages[0] || "התגובה לא מאוזנת נכון");
                    if (isBadFormat) currentInputErrors.push(errorMessages[1] || "בעיות במצבי צבירה או פורמט");
                }
                
                currentGrades.push(grade);
                // Join them with a line break so they display nicely
                currentErrors.push(currentInputErrors.join("<br>"));
            });
            
            // Write arrays to STACK inputs and dispatch change event to trigger saving
            let newAnsStr = JSON.stringify(currentAnswers);
            let newGradeStr = JSON.stringify([currentGrades, currentErrors]);
            
            if (hiddenInput1.value !== newAnsStr) {
                hiddenInput1.value = newAnsStr;
                hiddenInput1.dispatchEvent(new Event('change'));
            }
            if (hiddenInput2.value !== newGradeStr) {
                hiddenInput2.value = newGradeStr;
                hiddenInput2.dispatchEvent(new Event('change'));
            }
        }

        // Attempt to load saved answers if the student reopens the page before submitting
        let savedAnswers = [];
        if (hiddenInput1.value && hiddenInput1.value.trim() !== "") {
            try { 
                let parsed = JSON.parse(hiddenInput1.value);
                if (typeof parsed === 'string') parsed = JSON.parse(parsed);
                if (Array.isArray(parsed)) savedAnswers = parsed;
            } catch(e) {}
        }

        // Restore saved answers or inject starting prefixes
        mfs.forEach((mfObj, index) => {
            if (savedAnswers[index]) {
                mfObj.element.setValue(savedAnswers[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            } else if (prefixes[index]) {
                mfObj.element.setValue(prefixes[index].replace(/\\+/g, '\\'), {suppressChangeNotifications: true});
            }

            // Debounce student inputs before evaluating to prevent lag
            let debounceTimer;
            mfObj.element.addEventListener('input', () => {
                clearTimeout(debounceTimer);
                debounceTimer = setTimeout(() => {
                    evaluateStudent();   
                    updateTeacherBox();  
                }, 400); 
            });
        });

        // Fire initial check on page load if unlocked
        if (!isLocked) {
            evaluateStudent();
            updateTeacherBox();
        }

        // Post-submission execution state
        if (isLocked) {
            btnContainer.style.pointerEvents = "none"; 
            btnContainer.style.opacity = "0.6"; 
            
            let finalGrades = [];
            let finalErrors = [];
            
            // Parse returned grade strings from STACK input
            try { 
                let parsedAns2 = JSON.parse(hiddenInput2.value);
                if (Array.isArray(parsedAns2) && Array.isArray(parsedAns2[0])) {
                    finalGrades = parsedAns2[0]; 
                    finalErrors = parsedAns2[1] || [];
                } else {
                    finalGrades = parsedAns2; 
                }
            } catch(e) {}
            
            // Lock fields and show feedback icons
            mfs.forEach((mfObj, index) => {
                mfObj.element.readOnly = true; 
                let grade = finalGrades[index];
                let errorText = finalErrors[index];
                
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

                    if (errorText && errorText !== "") {
                        const errorDiv = document.createElement('div');
                        errorDiv.style.cssText = "color: #d32f2f; font-weight: bold; margin-top: 10px; direction: rtl; text-align: right; background-color: #ffebee; padding: 6px 12px; border-radius: 6px; border: 1px solid #ffcdd2; font-size: 0.95em;";
                        errorDiv.innerHTML = errorText; // Use innerHTML so the line breaks work!
                        mfObj.row.parentElement.appendChild(errorDiv);
                    }
                }
            });
        }
    }); 
}); 

// Helper to generate Maxima code for the teacher's copy-paste box
function updateTeacherBox() {
    const teacherBox = window.parent.document.getElementById('teacher-box');
    if (!teacherBox) return;

    let currentAnswers = mfs.map(mf => mf.element.getValue());
    let maximaArray = '["' + currentAnswers.join('", "') + '"]';
    maximaArray = maximaArray.replace(/\\/g, '\\\\');
    const newText = "👨‍🏫 Teacher Maxima Code (Copy & Paste):\n\n" + "ta: " + maximaArray + ";";

    if (teacherBox.innerText !== newText) { teacherBox.innerText = newText; }
}

// Auto-resizes the STACK iframe based on dynamic DOM content height
function autoResize() {
    try {
        const parentIframe = window.parent.document.getElementById(FRAME_ID);
        const parentHolder = window.parent.document.getElementById(FRAME_ID.replace('stack-iframe', 'stack-iframe-holder'));
        if (parentIframe && parentHolder) {
            const newHeight = (document.body.scrollHeight + 20) + 'px';
            parentIframe.style.height = newHeight;
            parentHolder.style.height = newHeight;
        }
    } catch (e) { }
}

autoResize();
const resizeObserver = new ResizeObserver(() => { autoResize(); });
resizeObserver.observe(document.body);
[[/script]]
[[/iframe]]
