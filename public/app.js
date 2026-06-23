    const hmTable = document.getElementById('hash-table');
    const llList = document.getElementById('linked-list');
    const term = document.getElementById('console');

    const ptrMap = {};
    function getPtr(key) {
        if(!ptrMap[key]) {
            ptrMap[key] = "0x" + Math.floor(Math.random()*0xFFFFFF).toString(16).toUpperCase().padStart(6, '0');
        }
        return ptrMap[key];
    }

    function getLogLine(msg, level="info", timeStr=null) {
        const time = timeStr || new Date().toISOString().split('T')[1].slice(0,-1);
        let prefix = '';
        let colorClass = '';
        if(level === 'ok') { prefix = '[OK] '; colorClass = 'log-ok'; }
        else if(level === 'err') { prefix = '[ERR]'; colorClass = 'log-err'; }
        else if(level === 'warn'){ prefix = '[WRN]'; colorClass = 'log-warn'; }
        else { prefix = '[INF]'; colorClass = 'log-info'; }
        return `<div class="log-line"><span class="log-time">${time}</span><span class="${colorClass}">${prefix} ${msg}</span></div>`;
    }

    function logTerminal(msg, level="info", timeStr=null) {
        term.insertAdjacentHTML('afterbegin', getLogLine(msg, level, timeStr));
    }

    function render(data) {
        document.getElementById('st-cap').innerText = `${data.size} / ${data.capacity}`;
        document.getElementById('st-hits').innerText = data.hits;
        document.getElementById('st-misses').innerText = data.misses;
        document.getElementById('st-evict').innerText = data.evictions;
        document.getElementById('st-ratio').innerText = data.hit_ratio.toFixed(2) + '%';

        const oldNodes = {};
        document.querySelectorAll('.ll-node').forEach(node => {
            const key = node.getAttribute('data-key');
            oldNodes[key] = node.getBoundingClientRect();
        });

        // Handle Ghost Nodes for Eviction/Removal Animation
        const newKeys = new Set(data.nodes.map(n => n.key));
        document.querySelectorAll('.ll-node').forEach(node => {
            const key = node.getAttribute('data-key');
            if (!newKeys.has(key) && !node.classList.contains('removing-node')) {
                const clone = node.cloneNode(true);
                const rect = node.getBoundingClientRect();
                const containerRect = llList.getBoundingClientRect();
                
                clone.style.position = 'absolute';
                clone.style.left = (rect.left - containerRect.left + llList.scrollLeft) + 'px';
                clone.style.top = (rect.top - containerRect.top) + 'px';
                clone.style.margin = '0';
                clone.style.zIndex = '0';
                clone.classList.add('removing-node');
                
                llList.appendChild(clone);
                setTimeout(() => clone.remove(), 400);
            }
        });

        const sortedKeys = [...data.nodes].map(n => n.key).sort();
        if(sortedKeys.length === 0) {
            hmTable.innerHTML = '<div style="color: var(--text-muted); font-size: 0.8rem; text-align:center; margin-top:20px;">[ MAP EMPTY ]</div>';
        } else {
            let hmHTML = '';
            sortedKeys.forEach(k => {
                hmHTML += `
                    <div class="hm-row">
                        <span class="hm-key">${k}</span>
                        <span class="hm-ptr">${getPtr(k)}</span>
                    </div>
                `;
            });
            hmTable.innerHTML = hmHTML;
        }

        if(data.nodes.length === 0) {
            llList.innerHTML = '<div style="color: var(--text-muted); font-family: var(--font-mono); font-size: 0.8rem;">[ LIST EMPTY ]</div>';
            return;
        }

        let llHTML = '';
        data.nodes.forEach((n, i) => {
            let cls = 'll-node';
            let tag = `IDX: ${i}`;
            if(i === 0) { cls += ' mru'; tag = 'HEAD (MRU)'; }
            else if(i === data.nodes.length - 1 && data.nodes.length > 1) { cls += ' lru'; tag = 'TAIL (LRU)'; }

            if (!oldNodes[n.key]) cls += ' new-node';

            llHTML += `
                <div class="${cls}" data-key="${n.key}">
                    <div style="font-size: 0.65rem; color: var(--accent-green); margin-bottom: 5px; text-align: right;">${getPtr(n.key)}</div>
                    <div class="node-header">
                        <span>${tag}</span>
                    </div>
                    <div class="node-key">${n.key}</div>
                    <div class="node-val">${n.value}</div>
                </div>
            `;
        });
        llList.innerHTML = llHTML;

        document.querySelectorAll('.ll-node').forEach(node => {
            const key = node.getAttribute('data-key');
            if (oldNodes[key]) {
                const oldRect = oldNodes[key];
                const newRect = node.getBoundingClientRect();
                const deltaX = oldRect.left - newRect.left;
                const deltaY = oldRect.top - newRect.top;

                node.style.transform = `translate(${deltaX}px, ${deltaY}px)`;
                node.style.transition = 'none';

                requestAnimationFrame(() => {
                    node.style.transition = 'transform 0.4s cubic-bezier(0.25, 0.8, 0.25, 1)';
                    node.style.transform = 'translate(0, 0)';
                });
            }
        });
    }

    async function reqApi(endpoint, query, successMsg, hitMissLogic = false) {
        try {
            const prevHits = parseInt(document.getElementById('st-hits').innerText);
            const res = await fetch(`/api/${endpoint}?${query}`, {method: 'POST'});
            if(!res.ok) throw new Error(await res.text());
            const data = await res.json();
            
            if (hitMissLogic) {
                if(data.hits > prevHits) logTerminal(`${successMsg} -> HIT`, 'ok');
                else logTerminal(`${successMsg} -> MISS`, 'err');
            } else {
                logTerminal(successMsg, 'ok');
            }
            render(data);
        } catch(e) {
            logTerminal(`REQ_FAIL: ${e.message}`, 'err');
        }
    }

    let currentOp = 'put';

    function setOp(op) {
        currentOp = op;
        document.querySelectorAll('.op-tab').forEach(b => b.classList.remove('active'));
        const btn = document.getElementById('btn-op-' + op);
        if (btn) btn.classList.add('active');

        const fgKey = document.getElementById('fg-key');
        const fgVal = document.getElementById('fg-val');
        const fgSim = document.getElementById('fg-sim');

        fgKey.style.display = 'none';
        fgVal.style.display = 'none';
        fgSim.style.display = 'none';

        if (op === 'put') {
            fgKey.style.display = 'block';
            fgVal.style.display = 'block';
            document.getElementById('inp-key').focus();
        } else if (op === 'get' || op === 'del') {
            fgKey.style.display = 'block';
            document.getElementById('inp-key').focus();
        } else if (op === 'sim') {
            fgSim.style.display = 'block';
            document.getElementById('inp-sim-ops').focus();
        }
    }

    function executeOp() {
        const op = currentOp;
        
        if (op === 'put') {
            const k = document.getElementById('inp-key').value;
            const v = document.getElementById('inp-val').value;
            if(!k || !v) return;
            reqApi('put', `key=${encodeURIComponent(k)}&value=${encodeURIComponent(v)}`, `PUT ${k}=${v}`);
            document.getElementById('inp-key').value = '';
            document.getElementById('inp-val').value = '';
            document.getElementById('inp-key').focus();
        } 
        else if (op === 'get') {
            const k = document.getElementById('inp-key').value;
            if(!k) return;
            reqApi('get', `key=${encodeURIComponent(k)}`, `GET ${k}`, true);
            document.getElementById('inp-key').value = '';
        }
        else if (op === 'del') {
            const k = document.getElementById('inp-key').value;
            if(!k) return;
            reqApi('remove', `key=${encodeURIComponent(k)}`, `DEL ${k}`);
            document.getElementById('inp-key').value = '';
        }
        else if (op === 'sim') {
            let o = parseInt(document.getElementById('inp-sim-ops').value) || 20;
            if (o > 100) o = 100;
            runSimulation(o);
        }
    }

    let simHistory = [];
    let simIndex = 0;
    let inSimulation = false;

    async function runSimulation(ops) {
        document.getElementById('console').innerHTML = '';
        logTerminal(`[SIM] Starting generation: Ops=${ops}`, 'info');
        
        const res = await fetch(`/api/state`);
        let currentState = await res.json();
        let cap = currentState.capacity;
        
        simHistory = [{
            state: currentState,
            logMsg: `[SIM] Captured existing cache state (CAP=${cap})`,
            logLvl: 'info',
            time: new Date().toISOString().split('T')[1].slice(0,-1)
        }];

        const possibleKeys = [
            "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa",
            "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega",
            "Apollo", "Athena", "Ares", "Hades", "Zeus", "Hera"
        ];

        let batchStr = "";
        let genHistory = [];
        let shadowCache = new Map();

        // Sync shadow cache with existing C++ state
        // currentState.nodes is [MRU, ..., LRU]
        // To make shadowCache have LRU at the start (for insertion order), iterate in reverse!
        for (let i = currentState.nodes.length - 1; i >= 0; i--) {
            shadowCache.set(currentState.nodes[i].key, true);
        }

        let lastOpType = null;
        let consecutiveCount = 0;

        for(let i=0; i<ops; i++) {
            let currentKeys = Array.from(shadowCache.keys());
            let opType, key;
            
            let fullness = shadowCache.size / cap;
            let r = Math.random();

            if (fullness < 0.9) {
                // FILL PHASE: Rapidly fill the cache
                if (r < 0.80) {
                    opType = "put";
                } else if (r < 0.95 && currentKeys.length > 0) {
                    opType = "get";
                } else {
                    opType = "del";
                }
            } else {
                // CHURN PHASE: Cache is full. Demonstrate Hit vs Eviction
                if (r < 0.65 && currentKeys.length > 0) {
                    opType = "get"; // 65% Cache Hit
                } else if (r < 0.85) {
                    opType = "put"; // 20% Insert (Causes Eviction)
                } else if (r < 0.95) {
                    opType = "get"; // 10% Miss
                } else {
                    opType = "del"; // 5% Delete
                }
            }

            // Enforce max 2 consecutive identical operations
            if (opType === lastOpType) {
                consecutiveCount++;
                if (consecutiveCount > 2) {
                    let altOps = ["put", "get", "del"].filter(op => op !== opType);
                    opType = altOps[Math.floor(Math.random() * altOps.length)];
                    lastOpType = opType;
                    consecutiveCount = 1;
                }
            } else {
                lastOpType = opType;
                consecutiveCount = 1;
            }

            // Key Selection Logic
            if (opType === "put") {
                const missing = possibleKeys.filter(k => !shadowCache.has(k));
                if (missing.length > 0 && Math.random() < 0.8) {
                    key = missing[Math.floor(Math.random() * missing.length)]; // New Insert
                } else if (currentKeys.length > 0) {
                    key = currentKeys[Math.floor(Math.random() * currentKeys.length)]; // Update
                } else {
                    key = possibleKeys[Math.floor(Math.random() * possibleKeys.length)];
                }
            } else if (opType === "get") {
                let wantHit = (fullness < 0.9) ? true : (r < 0.65);
                if (wantHit && currentKeys.length > 0) {
                    // Zipfian-like Hotspot: Favor recently added/accessed keys (end of array)
                    let idx = Math.floor(Math.pow(Math.random(), 2) * currentKeys.length);
                    key = currentKeys[currentKeys.length - 1 - idx];
                } else {
                    const missing = possibleKeys.filter(k => !shadowCache.has(k));
                    key = missing.length > 0 ? missing[Math.floor(Math.random() * missing.length)] : possibleKeys[0];
                }
            } else if (opType === "del") {
                let wantHit = Math.random() < 0.7; // Mostly green deletes
                if (wantHit && currentKeys.length > 0) {
                    key = currentKeys[Math.floor(Math.random() * currentKeys.length)];
                } else {
                    const missing = possibleKeys.filter(k => !shadowCache.has(k));
                    key = missing.length > 0 ? missing[Math.floor(Math.random() * missing.length)] : possibleKeys[0];
                }
            }

            // Update Shadow
            if (opType === "put") {
                shadowCache.delete(key);
                shadowCache.set(key, true);
                if (shadowCache.size > cap) shadowCache.delete(shadowCache.keys().next().value);
            } else if (opType === "get" && shadowCache.has(key)) {
                shadowCache.delete(key);
                shadowCache.set(key, true);
            } else if (opType === "del") {
                shadowCache.delete(key);
            }

            let msg = `[SIM ${i+1}/${ops}] `;
            if(opType === "put") {
                const val = Math.floor(Math.random() * 9000 + 1000).toString();
                batchStr += `P,${key},${val}|`;
                msg += `PUT ${key}=${val}`;
            } else if (opType === "get") {
                batchStr += `G,${key}|`;
                msg += `GET ${key}`;
            } else {
                batchStr += `R,${key}|`;
                msg += `DEL ${key}`;
            }
            genHistory.push({opType, key, msg});
        }

        const r = await fetch(`/api/batch?ops=${batchStr}`, {method: 'POST'});
        const statesArray = await r.json();

        let prevState = currentState;
        let prevHits = prevState.hits;

        for(let i=0; i<ops; i++) {
            const opType = genHistory[i].opType;
            const key = genHistory[i].key;
            let msg = genHistory[i].msg;
            
            currentState = statesArray[i];

            let lvl = 'ok';
            if (opType === 'get') {
                if (currentState.hits > prevHits) { msg += " -> HIT"; lvl = 'ok'; } 
                else { msg += " -> MISS"; lvl = 'err'; }
            } else if (opType === 'remove' || opType === 'del') {
                const keyExists = prevState.nodes.some(n => n.key === key);
                if (keyExists) { msg += " -> DELETED"; lvl = 'ok'; } 
                else { msg += " -> NOT_FOUND"; lvl = 'err'; }
            } else if (opType === 'put') {
                const keyExists = prevState.nodes.some(n => n.key === key);
                if (keyExists) { msg += " -> UPDATED"; lvl = 'info'; } 
                else {
                    if (prevState.nodes.length >= cap) { msg += " -> EVICTED_AND_INSERTED"; lvl = 'warn'; } 
                    else { msg += " -> INSERTED"; lvl = 'ok'; }
                }
            }

            simHistory.push({
                state: currentState,
                logMsg: msg,
                logLvl: lvl,
                time: new Date().toISOString().split('T')[1].slice(0,-1)
            });

            prevState = currentState;
            prevHits = currentState.hits;
        }

        inSimulation = true;
        simIndex = 0;
        document.getElementById('fg-sim').style.display = 'none';
        document.getElementById('btn-exec').style.display = 'none';
        document.getElementById('sim-playback').style.display = 'block';

        applySimState();
    }

    let autoPlayInterval = null;
    function toggleAutoPlay() {
        const btn = document.getElementById('btn-play');
        if (autoPlayInterval) {
            clearInterval(autoPlayInterval);
            autoPlayInterval = null;
            btn.innerText = "PLAY";
            btn.style.background = "var(--accent-blue)";
        } else {
            btn.innerText = "PAUSE";
            btn.style.background = "var(--accent-red)";
            autoPlayInterval = setInterval(() => {
                if (simIndex >= simHistory.length - 1) {
                    toggleAutoPlay();
                } else {
                    stepSim(1);
                }
            }, 600);
        }
    }

    function applySimState() {
        let html = "";
        // Iterate backwards so the most recent log is at the top of the terminal!
        for(let i = simIndex; i >= 0; i--) {
            html += getLogLine(simHistory[i].logMsg, simHistory[i].logLvl, simHistory[i].time);
        }
        document.getElementById('console').innerHTML = html;
        render(simHistory[simIndex].state);
        document.getElementById('sim-step-label').innerText = `Step ${simIndex} / ${simHistory.length - 1}`;
    }

    function stepSim(dir) {
        simIndex += dir;
        if(simIndex < 0) simIndex = 0;
        if(simIndex >= simHistory.length) simIndex = simHistory.length - 1;
        applySimState();
    }

    async function exitSim() {
        if (autoPlayInterval) toggleAutoPlay();
        inSimulation = false;
        document.getElementById('sim-playback').style.display = 'none';
        document.getElementById('btn-exec').style.display = 'block';
        setOp('put');
        
        const res = await fetch('/api/state');
        const state = await res.json();
        reqApi('reset', `capacity=${state.capacity}`, 'Simulation Exited - Cache Cleared');
        document.getElementById('console').innerHTML = '';
    }

    function bootSystem() {
        const c = document.getElementById('inp-boot-cap').value || 10;
        document.getElementById('init-overlay').style.display = 'none';
        reqApi('reset', `capacity=${c}`, `SYSTEM_BOOT CAP=${c}`);
        document.getElementById('inp-key').focus();
    }

    function showBootScreen() {
        if (inSimulation) {
            inSimulation = false;
            document.getElementById('sim-playback').style.display = 'none';
            document.getElementById('btn-exec').style.display = 'block';
        }
        
        document.getElementById('init-overlay').style.display = 'flex';
        document.getElementById('inp-boot-cap').value = ''; 
        document.getElementById('inp-boot-cap').focus();
        
        // Clear terminal
        document.getElementById('console').innerHTML = '';
        
        // Reset to normal PUT mode underneath
        setOp('put');
    }

    // Allow Enter key to submit on boot
    document.getElementById('inp-boot-cap').addEventListener('keypress', function(e) {
        if(e.key === 'Enter') bootSystem();
    });

    // Allow Enter key to submit or tab
    document.addEventListener('keypress', function (e) {
        if (document.getElementById('init-overlay').style.display !== 'none') return; // ignore if booting

        if (e.key === 'Enter') {
            const op = currentOp;
            if (op === 'put' && document.activeElement.id === 'inp-key') {
                document.getElementById('inp-val').focus();
            } else {
                executeOp();
            }
        }
    });
