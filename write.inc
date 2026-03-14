[[iframe width="100%" height="350px"]]

[[script src="https://unpkg.com/mathlive"]][[/script]]
[[style]]
    math-field::part(virtual-keyboard-toggle) {
      display: none !important;
    }
  [[/style]]

<div style="font-family: sans-serif; padding: 10px; box-sizing: border-box;">
    <div id="button-palette" style="background: white; border: 1px solid #ced4da; border-radius: 6px; margin-bottom: 15px; overflow: hidden; width: fit-content; max-width: 100%;">
        <div id="palette-title" style="padding: 8px 12px; background: #e9ecef; border-bottom: 1px solid #ced4da; font-weight: bold; font-size: 0.85em; color: #495057;">
            Symbols & Templates
        </div>
        <div id="preset-buttons-container" style="display: flex; flex-wrap: wrap; gap: 8px; padding: 10px;"></div>
    </div>
    <div style="border: 1px solid #ced4da; padding: 15px; border-radius: 4px; background: white;">
        <math-field id="chem-mathfield" style="width: 100%; font-size: 1.4em;"></math-field>
    </div>
</div>

[[script type="module"]]
import {stack_js} from '[[cors src="stackjsiframe.js"/]]';

try {
    let customTitle = {#js_title#};
    if (customTitle) document.getElementById('palette-title').innerText = customTitle;
} catch (e) { console.log("No custom title provided."); }

let texs = [];
try {
    texs = JSON.parse({#js_tex#});
} catch (e) { console.error("Failed to parse js_tex array:", e); }

let partialCredit = 0.5; 
try {
    let parsedPartial = parseFloat("{#js_partial_credit#}");
    if (!isNaN(parsedPartial)) {
        partialCredit = parsedPartial;
    }
} catch (e) { console.log("Could not parse js_partial_credit."); }

const btnContainer = document.getElementById('preset-buttons-container');
const mf = document.getElementById('chem-mathfield');

mf.parentNode.style.display = "flex";
mf.parentNode.style.alignItems = "center";

for (let i = 0; i < texs.length; i++) {
    let displayTex = Array.isArray(texs[i]) ? texs[i][0] : texs[i];
    let insertTex = Array.isArray(texs[i]) ? (texs[i][1] || texs[i][0]) : texs[i];
    
    const btn = document.createElement('div');
    btn.style.cssText = "position: relative; cursor: pointer; width: 85px; height: 45px; box-sizing: border-box; border: 1px solid #ced4da; border-radius: 4px; background-color: #f8f9fa; display: flex; align-items: center; justify-content: center; user-select: none; transition: all 0.2s;";
    const mathHtml = `<math-field read-only tabindex="-1" style="pointer-events: none; background: transparent; border: none; padding: 0; margin: 0; min-height: auto; font-size: 1.1em;">${displayTex}</math-field>`;
    const overlayHtml = `<div style="position: absolute; top: 0; left: 0; right: 0; bottom: 0; z-index: 100;"></div>`;
    btn.innerHTML = mathHtml + overlayHtml;
    
    btn.addEventListener('click', (e) => {
        e.preventDefault(); e.stopPropagation();
        mf.insert(insertTex); mf.focus();
    });
    btnContainer.appendChild(btn);
}

function processChem(latex) {
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
        return terms.map(t => t.replace(/\s+/g, '')).sort().join("+");
    }

    function makeSloppy(terms) {
        return terms.map(t => {
            return t.replace(/\\[a-zA-Z]+/g, '')
                    .replace(/[_^{}\(\)\[\]]/g, '')
                    .replace(/\s+/g, '')
                    .toLowerCase();
        }).sort().join("+");
    }

    let exactStr = makeExact(leftTerms) + theArrow + makeExact(rightTerms);
    let sloppyStr = makeSloppy(leftTerms) + "ARROW" + makeSloppy(rightTerms);

    return { exact: exactStr, sloppy: sloppyStr };
}

let rawTeacherAnswer = String.raw`{#ta#}`;
let cleanTeacherAnswer = rawTeacherAnswer.replace(/^["']|["']$/g, '').replace(/\\\\/g, '\\'); 
const teacherData = processChem(cleanTeacherAnswer);

stack_js.request_access_to_input("ans1", true).then((inputId1) => {
    const hiddenInput1 = document.getElementById(inputId1);
    
    stack_js.request_access_to_input("ans2", true).then((inputId2) => {
        const hiddenInput2 = document.getElementById(inputId2);
        
        const isLocked = hiddenInput1.readOnly || hiddenInput1.disabled || 
                         hiddenInput1.hasAttribute('readonly') || hiddenInput1.hasAttribute('disabled');
        
        if (isLocked) {
            mf.readOnly = true; 
            btnContainer.style.pointerEvents = "none"; 
            btnContainer.style.opacity = "0.6"; 
            
            // --- UPDATED VISUAL MARKING WITH RAW SVGs ---
            let finalGrade = parseFloat(hiddenInput2.value);
            
            if (!isNaN(finalGrade)) {
                const markIcon = document.createElement('div');
                markIcon.style.cssText = "margin-left: 15px; display: flex; align-items: center;";
                
                // SVG shapes perfectly matching the FontAwesome icons
                const svgCheck = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#4caf50"><path d="M470.6 105.4c12.5 12.5 12.5 32.8 0 45.3l-256 256c-12.5 12.5-32.8 12.5-45.3 0l-128-128c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0L192 338.7 425.4 105.4c12.5-12.5 32.8-12.5 45.3 0z"/></svg>`;
                const svgAdjust = `<svg viewBox="0 0 512 512" width="24" height="24" fill="#ff9800"><path d="M256 0C114.6 0 0 114.6 0 256s114.6 256 256 256 256-114.6 256-256S397.4 0 256 0zM256 464c-114.7 0-208-93.3-208-208S141.3 48 256 48v416z"/></svg>`;
                const svgTimes = `<svg viewBox="0 0 320 512" width="24" height="24" fill="#f44336"><path d="M310.6 150.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0L160 210.7 54.6 105.4c-12.5-12.5-32.8-12.5-45.3 0s-12.5 32.8 0 45.3L114.7 256 9.4 361.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L160 301.3 265.4 406.6c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L205.3 256 310.6 150.6z"/></svg>`;
                
                if (finalGrade === 1) {
                    markIcon.innerHTML = svgCheck; 
                } else if (finalGrade > 0 && finalGrade < 1) { 
                    markIcon.innerHTML = svgAdjust; 
                } else if (finalGrade === 0) {
                    markIcon.innerHTML = svgTimes; 
                }
                
                mf.parentNode.appendChild(markIcon);
            }
        }
        
        function evaluateStudent() {
            if (isLocked) return; 
            
            let rawLatex = mf.getValue("latex");
            
            let escapedLatex = rawLatex.replace(/\\/g, '\\\\');
            if (hiddenInput1.value !== escapedLatex) {
                hiddenInput1.value = escapedLatex;
                hiddenInput1.dispatchEvent(new Event('change'));
            }
            
            let studentData = processChem(rawLatex);
            let grade = 0;
            
            if (studentData.exact === teacherData.exact) {
                grade = 1; 
            } else if (studentData.sloppy === teacherData.sloppy) {
                grade = partialCredit; 
            } else {
                grade = 0; 
            }
            
            if (hiddenInput2.value !== grade.toString()) {
                hiddenInput2.value = grade.toString();
                hiddenInput2.dispatchEvent(new Event('change'));
            }
        }

        if (hiddenInput1.value && hiddenInput1.value.trim() !== "") {
            mf.setValue(hiddenInput1.value.replace(/\\\\/g, '\\'), {suppressChangeNotifications: true});
            evaluateStudent(); 
        }
        
        mf.addEventListener('input', evaluateStudent);

        let lastVal = hiddenInput1.value;
        setInterval(() => {
            if (hiddenInput1.value !== lastVal) {
                lastVal = hiddenInput1.value;
                
                let cleanLatex = lastVal.replace(/\\\\/g, '\\');
                
                if (mf.getValue("latex") !== cleanLatex) {
                    mf.setValue(cleanLatex, {suppressChangeNotifications: true});
                    evaluateStudent();
                }
            }
        }, 150);

    });
});
[[/script]]
[[/iframe]]
