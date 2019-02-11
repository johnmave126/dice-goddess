[@bs.deriving abstract]
type regexCapture('a) = {
  [@bs.optional] groups: 'a
};
external toGroupCapture: 'b => regexCapture('a) = "%identity";