type engine = unit => int;
type buffer;

[@bs.module "random-js"] external dice : (~numFaces: int, ~numDices: int) => (. engine) => array(int) = "dice";
[@bs.module "crypto"] external randomBytes : (int) => buffer = "randomBytes";
[@bs.send] external readInt32BE : (buffer, int) => int = "readInt32BE";

let cyptoEngine : engine = () => randomBytes(4) -> readInt32BE(0);

let cryptoDice = (numDices, numFaces) => Array.to_list(dice(~numDices=numDices, ~numFaces=numFaces)(. cyptoEngine));
