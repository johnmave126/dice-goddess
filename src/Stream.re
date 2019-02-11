type stream;

[@bs.send] external pipe : (stream, stream) => unit = "";
